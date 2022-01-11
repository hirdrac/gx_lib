//
// gx/Gui.cc
// Copyright (C) 2022 Richard Bradley
//

#include "Gui.hh"
#include "Window.hh"
#include "DrawContext.hh"
#include "Font.hh"
#include "Unicode.hh"
#include "System.hh"
#include "Logger.hh"
#include "Print.hh"
#include <algorithm>
#include <cassert>
using namespace gx;


[[nodiscard]] static int calcLines(std::string_view text)
{
  if (text.empty()) { return 0; }
  int lines = 1;
  for (int ch : text) { lines += (ch == '\n'); }
  return lines;
}

[[nodiscard]] static constexpr bool isMenu(GuiElemType type)
{
  return (type == GUI_MENU) || (type == GUI_SUBMENU);
}

[[nodiscard]] static constexpr bool isPopup(GuiElemType type)
{
  return (type == GUI_MENU) || (type == GUI_SUBMENU)
    || (type == GUI_LISTSELECT);
}

[[nodiscard]] static constexpr GuiElemType getPopupType(GuiElemType type)
{
  switch (type) {
    case GUI_MENU:
    case GUI_SUBMENU:
    case GUI_MENU_ITEM:
      return GUI_MENU;
    case GUI_LISTSELECT:
    case GUI_LISTSELECT_ITEM:
      return GUI_LISTSELECT;
    default:
      return GUI_NULL;
  }
}

[[nodiscard]] static constexpr bool isPopupItem(GuiElemType type)
{
  return (type == GUI_MENU_ITEM) || (type == GUI_LISTSELECT_ITEM);
}

static void deactivate(GuiElem& def)
{
  def._active = false;
  for (auto& e : def.elems) { deactivate(e); }
}

static bool activate(GuiElem& def, ElemID id)
{
  if (def._id == id) {
    def._active = true;
  } else {
    // activate parent if child is activated to handle nested menus
    for (auto& e : def.elems) { def._active |= activate(e, id); }
  }
  return def._active;
}

[[nodiscard]] static bool contains(const GuiElem& e, float x, float y)
{
  return (x >= e._x) && (x < (e._x + e._w))
    && (y >= e._y) && (y < (e._y + e._h));
}

[[nodiscard]] static GuiElem* findElemByXY(
  GuiElem& def, float x, float y, GuiElemType popupType)
{
  if (isPopup(def.type)) {
    if (popupType == GUI_NULL || (getPopupType(def.type) == popupType)) {
      // FIXME: shouldn't find listselect/listselect_item if outside of
      //   current listselect popup (ok for menus though)
      if (contains(def, x, y)) { return &def; }
      if (def._active) {
        GuiElem* e = findElemByXY(def.elems[1], x, y, GUI_NULL);
        if (e) { return e; }
      }
    }
  } else if ((def.eid != 0 || def.type == GUI_LISTSELECT_ITEM
              || def.type == GUI_TITLEBAR) && popupType == GUI_NULL
             && contains(def, x, y)) {
    return &def;
  } else {
    for (GuiElem& c : def.elems) {
      GuiElem* e = findElemByXY(c, x, y, popupType);
      if (e) { return e; }
    }

    if (def.type == GUI_PANEL && popupType == GUI_NULL
        && contains(def, x, y)) { return &def; }
  }
  return nullptr;
}

[[nodiscard]] static GuiElem* findItem(GuiElem& root, int no)
{
  for (auto& e : root.elems) {
    if (e.type == GUI_LISTSELECT_ITEM && (no == 0 || e.itemNo == no)) {
      return &e;
    } else if (!e.elems.empty()) {
      GuiElem* e2 = findItem(e, no);
      if (e2) { return e2; }
    }
  }
  return nullptr;
}

static void calcMaxItemSize(const GuiElem& root, float& maxW, float& maxH)
{
  for (auto& e : root.elems) {
    if (e.type == GUI_LISTSELECT_ITEM) {
      maxW = std::max(maxW, e._w);
      maxH = std::max(maxH, e._h);
      return;
    } else if (!e.elems.empty()) {
      calcMaxItemSize(e, maxW, maxH);
    }
  }
}

[[nodiscard]]
static GuiElem* findParentListSelect(GuiElem& def, int item_no)
{
  if (def.type == GUI_LISTSELECT) {
    return findItem(def.elems[1], item_no) ? &def : nullptr;
  } else if (!isMenu(def.type)) {
    for (auto& e : def.elems) {
      GuiElem* ptr = findParentListSelect(e, item_no);
      if (ptr) { return ptr; }
    }
  }

  return nullptr;
}

static bool addEntryChar(GuiElem& e, int32_t codepoint)
{
  assert(e.type == GUI_ENTRY);
  if (e.entry.maxLength != 0 && lengthUTF8(e.text) >= e.entry.maxLength) {
    return false; // no space for character
  }

  switch (e.entry.type) {
    default: // ENTRY_TEXT, ENTRY_PASSWORD
      if (codepoint <= 31) { return false; }
      break;
    case ENTRY_CARDINAL:
      if (!std::isdigit(codepoint)
          || (e.text == "0" && codepoint == '0')) { return false; }
      if (e.text == "0") { e.text.clear(); }
      break;
    case ENTRY_INTEGER:
      if ((!std::isdigit(codepoint) && codepoint != '-')
          || (codepoint == '-' && !e.text.empty() && e.text != "0")
          || (codepoint == '0' && (e.text == "0" || e.text == "-"))) {
        return false; }
      if (e.text == "0") { e.text.clear(); }
      break;
    case ENTRY_FLOAT:
      if ((!std::isdigit(codepoint) && codepoint != '-' && codepoint != '.')
          || (codepoint == '-' && !e.text.empty() && e.text != "0")
          || (codepoint == '0' && (e.text == "0" || e.text == "-0"))) {
        return false;
      } else if (codepoint == '.') {
        int count = 0;
        for (char ch : e.text) { count += int{ch == '.'}; }
        if (count > 0) { return false; }
      }
      if (e.text == "0" && codepoint != '.') { e.text.clear(); }
      break;
  }

  e.text += toUTF8(codepoint);
  return true;
}

static void drawRec(DrawContext& dc, float x, float y, float w, float h,
                    const GuiTheme::Style* style)
{
  if (style->backgroundColor != 0) {
    dc.color(style->backgroundColor);
    dc.rectangle(x, y, w, h);
  }

  if (style->edgeColor == 0) { return; }

  dc.color(style->edgeColor);
  switch (style->edgeType) {
    case GuiTheme::EDGE_BORDER_1px:
      dc.rectangle(x, y, w, 1);
      dc.rectangle(x, y + h - 1, w, 1);
      dc.rectangle(x, y+1, 1, h-2);
      dc.rectangle(x + w - 1, y+1, 1, h-2);
      break;
    case GuiTheme::EDGE_BORDER_2px:
      dc.rectangle(x, y, w, 2);
      dc.rectangle(x, y + h - 2, w, 2);
      dc.rectangle(x, y+2, 2, h-4);
      dc.rectangle(x + w - 2, y+2, 2, h-4);
      break;
    case GuiTheme::EDGE_UNDERLINE_1px:
      dc.rectangle(x, y + h - 1, w, 1);
      break;
    case GuiTheme::EDGE_UNDERLINE_2px:
      dc.rectangle(x, y + h - 2, w, 2);
      break;
  }
}

static void resizedElem(const GuiTheme& thm, GuiElem& def)
{
  // update children element sizes based on parent resize
  // (usually because of justify alignment)
  switch (def.type) {
    case GUI_HFRAME:
      for (GuiElem& e : def.elems) {
        if (e.align & ALIGN_VJUSTIFY) { e._h = def._h; resizedElem(thm, e); }
      }
      break;
    case GUI_VFRAME:
      for (GuiElem& e : def.elems) {
        if (e.align & ALIGN_HJUSTIFY) { e._w = def._w; resizedElem(thm, e); }
      }
      break;
    case GUI_SPACER:
      if (!def.elems.empty()) {
        GuiElem& e = def.elems[0];
        bool resized = false;
        if (e.align & ALIGN_HJUSTIFY) {
          resized = true;
          e._w = def._w - (def.spacer.left + def.spacer.right);
        }
        if (e.align & ALIGN_VJUSTIFY) {
          resized = true;
          e._h = def._h - (def.spacer.top + def.spacer.bottom);
        }
        if (resized) { resizedElem(thm, e); }
      }
      break;
    case GUI_PANEL: {
      GuiElem& e0 = def.elems[0];
      const float b2 = thm.panelBorder * 2.0f;
      e0._w = def._w - b2;
      e0._h = def._h - b2;
      resizedElem(thm, e0);
      break;
    }
    case GUI_MENU_FRAME: {
      GuiElem& e0 = def.elems[0];
      const float b2 = thm.menuFrameBorder * 2.0f;
      e0._w = def._w - b2;
      e0._h = def._h - b2;
      resizedElem(thm, e0);
      break;
    }
    case GUI_LISTSELECT: {
      // listselect popup list width
      GuiElem& e1 = def.elems[1]; // GUI_MENU_FRAME
      e1._w = def._w + (thm.menuFrameBorder * 2.0f);
      resizedElem(thm, e1);
      break;
    }
    default:
      break;
  }
}

static void calcSize(const GuiTheme& thm, GuiElem& def)
{
  const float b = thm.border;
  switch (def.type) {
    case GUI_HFRAME: {
      const float fs = thm.frameSpacing;
      float max_w = 0, max_h = 0;
      for (GuiElem& e : def.elems) {
        calcSize(thm, e);
        max_w = std::max(max_w, e._w);
        max_h = std::max(max_h, e._h);
      }
      float total_w = -fs;
      for (GuiElem& e : def.elems) {
        bool resized = false;
        if (e.align & ALIGN_HJUSTIFY) { e._w = max_w; resized = true; }
        if (e.align & ALIGN_VJUSTIFY) { e._h = max_h; resized = true; }
        if (resized) { resizedElem(thm, e); }
        total_w += e._w + fs;
      }
      def._w = total_w;
      def._h = max_h;
      break;
    }
    case GUI_VFRAME: {
      const float fs = thm.frameSpacing;
      float max_w = 0, max_h = 0;
      for (GuiElem& e : def.elems) {
        calcSize(thm, e);
        max_w = std::max(max_w, e._w);
        max_h = std::max(max_h, e._h);
      }
      float total_h = -fs;
      for (GuiElem& e : def.elems) {
        bool resized = false;
        if (e.align & ALIGN_HJUSTIFY) { e._w = max_w; resized = true; }
        if (e.align & ALIGN_VJUSTIFY) { e._h = max_h; resized = true; }
        if (resized) { resizedElem(thm, e); }
        total_h += e._h + fs;
      }
      def._w = max_w;
      def._h = total_h;
      break;
    }
    case GUI_SPACER:
      def._w = def.spacer.left + def.spacer.right;
      def._h = def.spacer.top + def.spacer.bottom;
      if (!def.elems.empty()) {
        GuiElem& e = def.elems[0];
        calcSize(thm, e);
        def._w += e._w;
        def._h += e._h;
      }
      break;
    case GUI_PANEL: {
      GuiElem& e = def.elems[0];
      calcSize(thm, e);
      const float b2 = thm.panelBorder * 2.0f;
      def._w = e._w + b2;
      def._h = e._h + b2;
      break;
    }
    case GUI_MENU_FRAME: {
      GuiElem& e = def.elems[0];
      calcSize(thm, e);
      const float b2 = thm.menuFrameBorder * 2.0f;
      def._w = e._w + b2;
      def._h = e._h + b2;
      break;
    }
    case GUI_TITLEBAR:
      if (def.elems.empty()) {
        def._w = thm.titlebarMinWidth;
        def._h = thm.titlebarMinHeight;
      } else {
        GuiElem& e = def.elems[0];
        calcSize(thm, e);
        def._w = e._w + (b * 2.0f);
        def._h = e._h + (b * 2.0f);
      }
      break;
    case GUI_LABEL: {
      const Font& fnt = *thm.font;
      def._w = fnt.calcWidth(def.text);
      const int lines = calcLines(def.text);
      def._h = float((fnt.size() - 1) * lines
                     + (thm.textSpacing * std::max(lines - 1, 0)));
      // FIXME: improve line height calc (based on font ymax/ymin?)
      break;
    }
    case GUI_HLINE:
      def._w = float(thm.font->size() - 1);
      def._h = float(thm.lineWidth) + (b * 2.0f);
      break;
    case GUI_VLINE:
      def._w = float(thm.lineWidth) + (b * 2.0f);
      def._h = float(thm.font->size() - 1);
      break;
    case GUI_BUTTON:
    case GUI_BUTTON_PRESS:
    case GUI_MENU_ITEM: {
      GuiElem& e = def.elems[0];
      calcSize(thm, e);
      def._w = e._w + (b * 2.0f);
      def._h = e._h + (b * 2.0f);
      break;
    }
    case GUI_CHECKBOX: {
      GuiElem& e = def.elems[0];
      calcSize(thm, e);
      const Font& fnt = *thm.font;
      def._w = fnt.calcWidth(thm.checkCode) + (b*3.0f) + e._w;
      def._h = std::max(float(fnt.size() - 1) + (b*2.0f), e._h);
      break;
    }
    case GUI_MENU: {
      // menu button
      GuiElem& e = def.elems[0];
      calcSize(thm, e);
      def._w = e._w + (b * 2.0f);
      def._h = e._h + (b * 2.0f);
      // menu items
      calcSize(thm, def.elems[1]);
      break;
    }
    case GUI_SUBMENU: {
      // menu header
      GuiElem& e = def.elems[0];
      calcSize(thm, e);
      def._w = e._w + (b * 3.0f) + thm.font->calcWidth(thm.subMenuCode);
      def._h = e._h + (b * 2.0f);
      // menu items
      calcSize(thm, def.elems[1]);
      break;
    }
    case GUI_LISTSELECT: {
      calcSize(thm, def.elems[0]);
      GuiElem& e1 = def.elems[1]; // GUI_MENU_FRAME
      calcSize(thm, e1);
      def._w = 0; def._h = 0;
      calcMaxItemSize(e1.elems[0], def._w, def._h);
      break;
    }
    case GUI_LISTSELECT_ITEM: {
      GuiElem& e = def.elems[0];
      calcSize(thm, e);
      def._w = e._w + (b * 3.0f) + thm.font->calcWidth(thm.listSelectCode);
      def._h = e._h + (b * 2.0f);
      break;
    }
    case GUI_ENTRY: {
      const Font& fnt = *thm.font;
      if (def.entry.type == ENTRY_CARDINAL
          || def.entry.type == ENTRY_INTEGER
          || def.entry.type == ENTRY_FLOAT) {
        def._w = def.entry.size * fnt.digitWidth();
      } else {
        def._w = def.entry.size * fnt.calcWidth("A");
        // FIXME: use better width value than capital A * size
      }
      def._w += float(thm.entryLeftMargin + thm.entryRightMargin
                      + thm.cursorWidth + 1);
      def._h = float(fnt.size() - 1)
        + thm.entryTopMargin + thm.entryBottomMargin;
      break;
    }
    case GUI_IMAGE:
      def._w = def.image.width + (b * 2.0f);
      def._h = def.image.height + (b * 2.0f);
      break;
    default:
      GX_LOG_ERROR("unknown type ", def.type);
      break;
  }
}

static void calcPos(const GuiTheme& thm, GuiElem& def,
                    float left, float top, float right, float bottom)
{
  switch (VAlign(def.align)) {
    case ALIGN_TOP:    def._y = top; break;
    case ALIGN_BOTTOM: def._y = bottom - def._h; break;
    default: // vcenter
      def._y = std::floor((top + bottom - def._h) * .5f + .5f); break;
  }

  switch (HAlign(def.align)) {
    case ALIGN_LEFT:  def._x = left; break;
    case ALIGN_RIGHT: def._x = right - def._w; break;
    default: // hcenter
      def._x = std::floor((left + right - def._w) * .5f); break;
  }

  left   = def._x;
  top    = def._y;
  right  = def._x + def._w;
  bottom = def._y + def._h;

  switch (def.type) {
    case GUI_HFRAME: {
      const float fs = thm.frameSpacing;
      float total_w = 0;
      for (GuiElem& e : def.elems) { total_w += e._w + fs; }
      for (GuiElem& e : def.elems) {
        total_w -= e._w + fs;
        calcPos(thm, e, left, top, right - total_w, bottom);
        left = e._x + e._w + fs;
      }
      break;
    }
    case GUI_VFRAME: {
      const float fs = thm.frameSpacing;
      float total_h = 0;
      for (GuiElem& e : def.elems) { total_h += e._h + fs; }
      for (GuiElem& e : def.elems) {
        total_h -= e._h + fs;
        calcPos(thm, e, left, top, right, bottom - total_h);
        top = e._y + e._h + fs;
      }
      break;
    }
    case GUI_SPACER:
      if (!def.elems.empty()) {
        calcPos(thm, def.elems[0], left + def.spacer.left,
                top + def.spacer.top, right - def.spacer.right,
                bottom - def.spacer.bottom);
      }
      break;
    case GUI_PANEL: {
      const float b = thm.panelBorder;
      calcPos(thm, def.elems[0], left + b, top + b, right - b, bottom - b);
      break;
    }
    case GUI_MENU_FRAME: {
      const float b = thm.menuFrameBorder;
      calcPos(thm, def.elems[0], left + b, top + b, right - b, bottom - b);
      break;
    }
    case GUI_CHECKBOX:
      left += thm.font->calcWidth(thm.checkCode) + (thm.border*3.0f);
      calcPos(thm, def.elems[0], left, top, right, bottom);
      break;
    case GUI_MENU: {
      GuiElem& e0 = def.elems[0];
      calcPos(thm, e0, left, top, right, bottom);
      // always position menu frame below menu button for now
      top  = e0._y + e0._h + thm.border;
      GuiElem& e1 = def.elems[1];
      calcPos(thm, e1, left, top, left + e1._w, top + e1._h);
      break;
    }
    case GUI_SUBMENU: {
      const float b = thm.border;
      calcPos(thm, def.elems[0], left + b, top + b, right - b, bottom - b);
      // sub-menu items
      left += def._w;
      GuiElem& e1 = def.elems[1];
      calcPos(thm, e1, left, top, left + e1._w, top + e1._h);
      break;
    }
    case GUI_LISTSELECT: {
      const float b = thm.border;
      calcPos(thm, def.elems[0], left + b, top + b, right - b, bottom - b);
      calcPos(thm, def.elems[1], left - thm.menuFrameBorder, top + def._h,
              right, bottom);
      break;
    }
    default:
      // align single child element
      if (!def.elems.empty()) {
        assert(def.elems.size() == 1);
        const float b = thm.border;
        calcPos(thm, def.elems[0], left + b, top + b, right - b, bottom - b);
      }
      break;
  }
}

static std::string passwordStr(int32_t code, std::size_t len)
{
  const std::string ch = toUTF8(code);
  std::string val;
  val.reserve(ch.size() * len);
  for (std::size_t i = 0; i < len; ++i) { val += ch; }
  return val;
}

template<class T>
static inline T* findElemByIDT(T* root, ElemID id)
{
  assert(id != 0);
  std::vector<T*> stack;
  T* e = root;
  while (e->_id != id) {
    if (!e->elems.empty()) {
      stack.reserve(stack.size() + e->elems.size());
      for (T& c : e->elems) { stack.push_back(&c); }
    }

    if (stack.empty()) { return nullptr; }
    e = stack.back();
    stack.pop_back();
  }
  return e;
}

template<class T>
static inline T* findElemByEventIDT(T* root, EventID eid)
{
  assert(eid != 0);
  std::vector<T*> stack;
  T* e = root;
  while (e->eid != eid) {
    if (!e->elems.empty()) {
      stack.reserve(stack.size() + e->elems.size());
      for (T& c : e->elems) { stack.push_back(&c); }
    }

    if (stack.empty()) { return nullptr; }
    e = stack.back();
    stack.pop_back();
  }
  return e;
}


// **** Gui class ****
void Gui::clear()
{
  _panels.clear();
  clearHeld();
  _hoverID = 0;
  _focusID = 0;
  _popupID = 0;
  _eventID = 0;
  _eventType = GUI_NULL;
  _eventTime = 0;
  _needRender = true;
  _textChanged = false;
}

void Gui::deletePanel(PanelID id)
{
  for (auto i = _panels.begin(), end = _panels.end(); i != end; ++i) {
    if ((*i)->id == id) {
      _panels.erase(i);
      _needRender = true;
      break;
    }
  }
}

void Gui::raisePanel(PanelID id)
{
  if (id == topPanel()) { return; }
  for (auto i = _panels.begin(), end = _panels.end(); i != end; ++i) {
    if ((*i)->id == id) {
      auto pPtr = std::move(*i);
      _panels.erase(i);
      _panels.insert(_panels.begin(), std::move(pPtr));
      _needRender = true;
      break;
    }
  }
}

void Gui::lowerPanel(PanelID id)
{
  if (id == bottomPanel()) { return; }
  for (auto i = _panels.begin(), end = _panels.end(); i != end; ++i) {
    if ((*i)->id == id) {
      auto pPtr = std::move(*i);
      _panels.erase(i);
      _panels.insert(_panels.end(), std::move(pPtr));
      _needRender = true;
      break;
    }
  }
}

bool Gui::getPanelLayout(PanelID id, Rect& layout) const
{
  Panel* p = nullptr;
  for (auto& pPtr : _panels) { if (pPtr->id == id) { p = pPtr.get(); break; } }
  if (!p) { return false; }

  layout = p->layout;
  return true;
}

void Gui::update(Window& win)
{
  const int64_t now = win.lastPollTime();

  // clear state that only persists for a single update
  _eventID = 0;
  _eventType = GUI_NULL;
  _needRedraw = false;

  for (auto& pPtr : _panels) {
    Panel& p = *pPtr;
    // size & position update
    if (p.needLayout) {
      calcSize(*p.theme, p.root);
      calcPos(*p.theme, p.root, 0, 0, p.layout.w, p.layout.h);
      p.needLayout = false;
    }
  }

  // mouse movement/button handling
  if (win.focused()) {
    if (win.allEvents() & (EVENT_MOUSE_MOVE | EVENT_MOUSE_BUTTON1)) {
      processMouseEvent(win);
    }

    if (_eventID == 0 && _heldID != 0 && _repeatDelay >= 0
        && (now - _heldTime) > _repeatDelay) {
      if (_repeatDelay > 0) {
        _heldTime += _repeatDelay * ((now - _heldTime) / _repeatDelay);
      }

      const GuiElem* heldElem = findElemByID(_heldID);
      if (heldElem) {
        _eventID = heldElem->eid;
        _eventType = _heldType;
        _eventTime = now;
      }
    }
  }

  // entry input handling & cursor update
  if (_focusID != 0) {
    if (win.events() & EVENT_CHAR) { processCharEvent(win); }

    if (_cursorBlinkTime > 0) {
      // check for cursor blink
      const int64_t blinks = (now - _lastCursorUpdate) / _cursorBlinkTime;
      if (blinks > 0) {
        _lastCursorUpdate += blinks * _cursorBlinkTime;
        if (blinks & 1) {
          _cursorState = !_cursorState;
          _needRender = true;
        }
      }
    }
  }

  // redraw GUI if needed
  if (_needRender) {
    DrawContext dc{_dl}, dc2{_dl2};
    dc.clear();
    dc2.clear();
    _needRender = false;

    for (auto it = _panels.rbegin(), end = _panels.rend(); it != end; ++it) {
      Panel& p = **it;
      _needRender |= drawElem(p, p.root, dc, dc2, now, &(p.theme->panel));

      if (!dc2.empty()) {
        dc.append(dc2);
        dc2.clear();
      }
    }

    if (_popupID != 0) {
      for (auto it = _panels.rbegin(), end = _panels.rend(); it != end; ++it) {
        Panel& p = **it;
        _needRender |= drawPopup(p, p.root, dc, dc2, now);

        if (!dc2.empty()) {
          dc.append(dc2);
          dc2.clear();
        }
      }
    }
    _needRedraw = true;
  }
}

void Gui::deactivatePopups()
{
  for (auto& pPtr : _panels) { deactivate(pPtr->root); }
  _popupID = 0;
  _needRender = true;
}

void Gui::activatePopup(ElemID id)
{
  assert(id != 0);
  if (_popupID != 0) { deactivatePopups(); }
  for (auto& pPtr : _panels) { if (activate(pPtr->root, id)) break; }
  _popupID = id;
  _needRender = true;
}

void Gui::processMouseEvent(Window& win)
{
  const bool buttonDown = win.buttons() & BUTTON1;
  const bool buttonEvent = win.events() & EVENT_MOUSE_BUTTON1;
  const bool pressEvent = buttonDown & buttonEvent;
  const bool anyGuiButtonEvent = win.allEvents() & EVENT_MOUSE_BUTTON1;

  GuiElemType popupType = GUI_NULL;
  if (_popupID != 0) {
    GuiElem* e = findElemByID(_popupID);
    popupType = getPopupType(e->type);
  }

  // get elem at mouse pointer
  Panel* pPtr = nullptr;
  GuiElem* ePtr = nullptr;
  ElemID id = 0;
  GuiElemType type = GUI_NULL;
  if (win.mouseIn()) {
    for (auto& ptr : _panels) {
      const Panel& p = *ptr;
      const float mx = win.mouseX() - p.layout.x;
      const float my = win.mouseY() - p.layout.y;
      ePtr = findElemByXY(ptr->root, mx, my, popupType);
      if (ePtr) {
        if (ePtr->_enabled) {
          id = ePtr->_id;
          type = ePtr->type;
          pPtr = ptr.get();
        }

        if (buttonEvent) { win.removeEvent(EVENT_MOUSE_BUTTON1); }
        break;
      }
    }
  }

  // update focus
  // FIXME: setFocusID() could trigger an event that is overridden below
  if (pressEvent) {
    setFocusID(win, (type == GUI_ENTRY) ? id : 0);
  } else if (buttonDown && anyGuiButtonEvent) {
    // click in other Gui instance clears our focus
    setFocusID(win, 0);
  }

  // update hoverID
  if (_hoverID != id) {
    const ElemID hid =
      (!buttonDown || isPopupItem(type) || id == _heldID) ? id : 0;
    if (_hoverID != hid) {
      _hoverID = hid;
      _needRender = true;
    }
  }

  if (buttonDown && _heldType == GUI_TITLEBAR) {
    if (win.events() & EVENT_MOUSE_MOVE) {
      pPtr = nullptr;
      for (auto& ptr : _panels) {
        if (findElemByIDT(&ptr->root, _heldID)) { pPtr = ptr.get(); break; }
      }
      assert(pPtr != nullptr);
      raisePanel(pPtr->id);
      const float mx = std::clamp(win.mouseX(), 0.0f, float(win.width()));
      const float my = std::clamp(win.mouseY(), 0.0f, float(win.height()));
      pPtr->layout.x += mx - _heldX;
      pPtr->layout.y += my - _heldY;
      _heldX = mx;
      _heldY = my;
      _needRender = true;
    }
  } else if (isPopup(type)) {
    if (pressEvent && ePtr->_active) {
      // click on open menu/listselect button closes popup
      deactivatePopups();
    } else if (pressEvent || (isMenu(type) && popupType == GUI_MENU)) {
      // open menu/listselect with click OR open menu/sub-menu with mouse-over
      if (_popupID != id) { activatePopup(id); }
    }
  } else if (type == GUI_MENU_ITEM) {
    if (buttonEvent) {
      _eventID = ePtr->eid;
      _eventType = type;
      _eventTime = win.lastPollTime();
      deactivatePopups();
    } else if (_popupID != id) {
      // activate on menu item to close sub-menus if necessary
      activatePopup(id);
    }
  } else if (type == GUI_LISTSELECT_ITEM) {
    if (buttonEvent) {
      GuiElem* parent = findParentListSelect(pPtr->root, ePtr->itemNo);
      if (parent) {
        GuiElem& ls = *parent;
        ls.itemNo = ePtr->itemNo;
        GuiElem& e0 = ls.elems[0];
        e0 = ePtr->elems[0];
        const float b = pPtr->theme->border;
        calcPos(*pPtr->theme, e0, ls._x + b, ls._y + b, ls._x + ls._w - b,
                ls._y + ls._h - b);
        _eventID = ls.eid;
        _eventType = ls.type;
        _eventTime = win.lastPollTime();
        deactivatePopups();
      }
    }
  } else {
    if (pressEvent && id != 0) {
      _heldID = id;
      _heldType = type;
      _heldTime = win.lastPollTime();
      _heldX = win.mouseX();
      _heldY = win.mouseY();
      _repeatDelay = (type == GUI_BUTTON_PRESS) ? ePtr->repeatDelay : -1;
      _needRender = true;
      if (type == GUI_BUTTON_PRESS) {
        _eventID = ePtr->eid;
        _eventType = type;
        _eventTime = win.lastPollTime();
      }
    } else if ((_heldType == GUI_BUTTON_PRESS) && (_heldID != id)) {
      // clear hold if cursor moves off BUTTON_PRESS
      clearHeld();
      _needRender = true;
    } else if (!buttonDown && (_heldID != 0)) {
      if ((type == GUI_BUTTON || type == GUI_CHECKBOX)
          && buttonEvent && (_heldID == id)) {
        // activate if cursor is over element & button is released
        _eventID = ePtr->eid;
        _eventType = type;
        _eventTime = win.lastPollTime();
        if (type == GUI_CHECKBOX) { ePtr->checkboxSet = !ePtr->checkboxSet; }
      }

      clearHeld();
      _needRender = true;
    }

    if (_popupID != 0 && anyGuiButtonEvent) {
      deactivatePopups();
    }
  }
}

void Gui::processCharEvent(Window& win)
{
  GuiElem* e = findElemByID(_focusID);
  if (!e) { return; }
  assert(e->type == GUI_ENTRY);

  bool usedEvent = false;
  for (const CharInfo& c : win.charData()) {
    if (c.codepoint) {
      if (addEntryChar(*e, int32_t(c.codepoint))) {
        usedEvent = true;
        _needRender = true;
        _textChanged = true;
        //println("char: ", c.codepoint);
      }
      // TODO: flash 'error' color if char isn't added
    } else if (c.key == KEY_BACKSPACE) {
      usedEvent = true;
      if (!e->text.empty()) {
        if (c.mods == MOD_ALT) {
          e->text.clear();
        } else {
          popbackUTF8(e->text);
        }
        _needRender = true;
        _textChanged = true;
        //println("backspace");
      }
    } else if (c.key == KEY_V && c.mods == MOD_CONTROL) {
      // (CTRL-V) paste first line of clipboard
      usedEvent = true;
      const std::string cb = getClipboardFirstLine();
      bool added = false;
      for (UTF8Iterator itr{cb}; !itr.done(); itr.next()) {
        added |= addEntryChar(*e, itr.get());
      }
      _needRender |= added;
      _textChanged |= added;
    } else if ((c.key == KEY_TAB && c.mods == 0) || c.key == KEY_ENTER) {
      const GuiElem* next = findNextElem(_focusID, GUI_ENTRY);
      setFocusID(win, next ? next->_id : 0);
      usedEvent = true;
    } else if (c.key == KEY_TAB && c.mods == MOD_SHIFT) {
      const GuiElem* prev = findPrevElem(_focusID, GUI_ENTRY);
      setFocusID(win, prev ? prev->_id : 0);
      usedEvent = true;
    }
    // TODO: handle KEY_LEFT, KEY_RIGHT for cursor movement
  }

  if (usedEvent) {
    win.removeEvent(EVENT_CHAR);
    // reset cursor blink state
    _lastCursorUpdate = win.lastPollTime();
    _needRender |= !_cursorState;
    _cursorState = true;
  }
}

void Gui::setFocusID(Window& win, ElemID id)
{
  if (_focusID == id) { return; }

  if (_textChanged) {
    _textChanged = false;
    const GuiElem* focusElem = findElemByID(_focusID);
    if (focusElem) {
      _eventID = focusElem->eid;
      _eventType = GUI_ENTRY;
      _eventTime = win.lastPollTime();
    }
  }

  _focusID = id;
  if (id != 0) {
    _lastCursorUpdate = win.lastPollTime();
    const Panel* p = nullptr;
    for (auto& pPtr : _panels) {
      if (findElemByIDT(&pPtr->root, _focusID)) { p = pPtr.get(); break; }
    }

    assert(p != nullptr);
    _cursorBlinkTime = p ? p->theme->cursorBlinkTime : 400000;
    _cursorState = true;
  }
  _needRender = true;
}

void Gui::setElemState(EventID eid, bool enable)
{
  GuiElem* e = findElemByEventID(eid);
  if (e && e->_enabled != enable) {
    e->_enabled = enable;
    _needRender = true;
  }
}

bool Gui::setText(EventID eid, std::string_view text)
{
  for (auto& pPtr : _panels) {
    GuiElem* e = findElemByEventIDT(&pPtr->root, eid);
    if (!e) { continue; }

    e->text = text;
    pPtr->needLayout |= (e->type != GUI_ENTRY);
    _needRender = true;
    return true;
  }
  return false;
}

bool Gui::setBool(EventID eid, bool val)
{
  for (auto& pPtr : _panels) {
    GuiElem* e = findElemByEventIDT(&pPtr->root, eid);
    if (!e) { continue; }

    if (e->type != GUI_CHECKBOX) { break; }
    e->checkboxSet = val;
    _needRender = true;
    return true;
  }
  return false;
}

bool Gui::setItemNo(EventID eid, int no)
{
  for (auto& pPtr : _panels) {
    GuiElem* e = findElemByEventIDT(&pPtr->root, eid);
    if (!e) { continue; }

    if (e->type != GUI_LISTSELECT) { break; }
    e->itemNo = no;
    _needRender = true;
    return true;
  }
  return false;
}

PanelID Gui::addPanel(
  std::unique_ptr<Panel> ptr, float x, float y, AlignEnum align)
{
  const PanelID id = ++_lastPanelID;
  ptr->id = id;
  initElem(ptr->root);
  layout(*ptr, x, y, align);
  _panels.insert(_panels.begin(), std::move(ptr));
  return id;
}

void Gui::layout(Panel& p, float x, float y, AlignEnum align)
{
  const GuiTheme& thm = *p.theme;
  assert(thm.font != nullptr);
  p.layout = {x,y,0,0};
  if (HAlign(align) == ALIGN_RIGHT) { std::swap(p.layout.x, p.layout.w); }
  if (VAlign(align) == ALIGN_BOTTOM) { std::swap(p.layout.y, p.layout.h); }
  p.needLayout = false;
  p.root.align = align;
  calcSize(thm, p.root);
  calcPos(thm, p.root, 0, 0, p.layout.w, p.layout.h);
  _needRender = true;
}

void Gui::initElem(GuiElem& def)
{
  def._id = ++_lastUniqueID;
  if (def.type == GUI_LISTSELECT) {
    GuiElem* e = findItem(def, def.itemNo);
    if (e) {
      GuiElem& e0 = def.elems[0];
      e0 = e->elems[0];
      e0.align = ALIGN_CENTER_LEFT;
      def.itemNo = e->itemNo;
    }
  }
  for (GuiElem& e : def.elems) { initElem(e); }
}

bool Gui::drawElem(
  const Panel& p, GuiElem& def, DrawContext& dc, DrawContext& dc2,
  int64_t usec, const GuiTheme::Style* style) const
{
  const GuiTheme& thm = *p.theme;
  const float ex = def._x + p.layout.x;
  const float ey = def._y + p.layout.y;
  const float ew = def._w;
  const float eh = def._h;

  bool needRedraw = false; // use for anim trigger later
  switch (def.type) {
    case GUI_PANEL:
    case GUI_MENU_FRAME:
      assert(style != nullptr);
      drawRec(dc, ex, ey, ew, eh, style);
      break;
    case GUI_TITLEBAR:
      drawRec(dc, ex, ey, ew, eh, &thm.titlebar);
      break;
    case GUI_LABEL:
      assert(style != nullptr);
      dc2.color(style->textColor);
      dc2.text(TextFormatting{thm.font, float(thm.textSpacing)},
               ex, ey, ALIGN_TOP_LEFT, def.text);
      break;
    case GUI_HLINE: {
      const float b = thm.border;
      assert(style != nullptr);
      dc.color(style->textColor);
      dc.rectangle(ex, ey + b, ew, eh - (b*2));
      break;
    }
    case GUI_VLINE: {
      const float b = thm.border;
      assert(style != nullptr);
      dc.color(style->textColor);
      dc.rectangle(ex + b, ey, ew - (b*2), eh);
      break;
    }
    case GUI_BUTTON:
    case GUI_BUTTON_PRESS:
      if (!def._enabled) {
        style = &thm.buttonDisable;
      } else if (def._id == _heldID) {
        style = (def._id == _hoverID || def.type != GUI_BUTTON)
          ? &thm.buttonPress : &thm.buttonHold;
      } else {
        style = (def._id == _hoverID) ? &thm.buttonHover : &thm.button;
      }
      drawRec(dc, ex, ey, ew, eh, style);
      break;
    case GUI_CHECKBOX: {
      if (!def._enabled) {
        style = &thm.checkboxDisable;
      } else if (def._id == _heldID) {
        style = (def._id == _hoverID) ? &thm.checkboxPress : &thm.checkboxHold;
      } else {
        style = (def._id == _hoverID) ? &thm.checkboxHover : &thm.checkbox;
      }
      const float b = thm.border;
      const float cw = thm.font->calcWidth(thm.checkCode) + (b*2.0f);
      const float ch = float(thm.font->size() - 1) + (b*2.0f);
      drawRec(dc, ex, ey, cw, ch, style);
      if (def.checkboxSet) {
        dc2.color(style->textColor);
        dc2.glyph(TextFormatting{thm.font, float(thm.textSpacing)},
                  ex + b + thm.checkXOffset, ey + b + thm.checkYOffset,
                  ALIGN_TOP_LEFT, thm.checkCode);
      }
      break;
    }
    case GUI_MENU:
      style = def._active ? &thm.menuButtonOpen
        : ((def._id == _hoverID) ? &thm.menuButtonHover : &thm.menuButton);
      drawRec(dc, ex, ey, ew, eh, style);
      needRedraw |= drawElem(p, def.elems[0], dc, dc2, usec, style);
      break;
    case GUI_MENU_ITEM:
      if (!def._enabled) {
        style = &thm.menuItemDisable;
      } else if (def._id == _hoverID) {
        style = &thm.menuItemSelect;
        drawRec(dc, ex, ey, ew, eh, style);
      }
      break;
    case GUI_SUBMENU: {
      if (def._active) {
        style = &thm.menuItemSelect;
        drawRec(dc, ex, ey, ew, eh, style);
      }
      needRedraw |= drawElem(p, def.elems[0], dc, dc2, usec, style);
      const float b = thm.border;
      dc2.glyph(TextFormatting{thm.font, 0}, ex + ew, ey + b,
                ALIGN_TOP_RIGHT, thm.subMenuCode);
      break;
    }
    case GUI_LISTSELECT: {
      if (!def._enabled) {
        style = &thm.listSelectDisable;
      } else if (def._active) {
        style = &thm.listSelectOpen;
      } else {
        style = (def._id == _hoverID) ? &thm.listSelectHover : &thm.listSelect;
      }
      drawRec(dc, ex, ey, ew, eh, style);
      needRedraw |= drawElem(p, def.elems[0], dc, dc2, usec, style);
      const float b = thm.border;
      dc2.glyph(TextFormatting{thm.font, 0}, ex + ew - b, ey + b,
                ALIGN_TOP_RIGHT, thm.listSelectCode);
      break;
    }
    case GUI_LISTSELECT_ITEM:
      if (!def._enabled) {
        style = &thm.listSelectItemDisable;
      } else if (def._id == _hoverID) {
        style = &thm.listSelectItemSelect;
        drawRec(dc, ex, ey, ew, eh, style);
      }
      break;
    case GUI_ENTRY: {
      if (!def._enabled) {
        style = &thm.entryDisable;
      } else {
        style = (def._id == _focusID) ? &thm.entryFocus : &thm.entry;
      }
      drawRec(dc, ex, ey, ew, eh, style);
      const std::string txt = (def.entry.type == ENTRY_PASSWORD)
        ? passwordStr(thm.passwordCode, def.text.size()) : def.text;
      const RGBA8 textColor = style->textColor;
      const float cw = thm.cursorWidth;
      const float tw = thm.font->calcWidth(txt);
      const float fs = float(thm.font->size());
      const float maxWidth = ew - thm.entryLeftMargin
        - thm.entryRightMargin - cw;
      float tx = ex + thm.entryLeftMargin;
      if (tw > maxWidth) {
        // text doesn't fit in entry
        const RGBA8 c0 = textColor & 0x00ffffff;
        if (def._id == _focusID) {
          // text being edited so show text end where cursor is
          dc2.hgradient(ex, c0, tx + (fs * .5f), textColor);
          tx -= tw - maxWidth;
        } else {
          // show text start when not in focus
          dc2.hgradient(ex + ew - thm.entryRightMargin - (fs * .5f),
                        textColor, ex + ew, c0);
        }
      } else {
        dc2.color(textColor);
        if (HAlign(def.entry.align) == ALIGN_RIGHT) {
          tx = ex + ew - (tw + cw + thm.entryRightMargin);
        } else if (HAlign(def.entry.align) != ALIGN_LEFT) { // HCENTER
          tx = ex + ((ew - tw) * .5f);
        }
      }
      dc2.text(TextFormatting{thm.font, float(thm.textSpacing)},
               tx, ey + thm.entryTopMargin, ALIGN_TOP_LEFT, txt,
               {ex, ey, ew, eh});
      if (def._id == _focusID && _cursorState) {
        // draw cursor
        dc.color(thm.cursorColor);
        dc.rectangle(tx + tw, ey + thm.entryTopMargin, cw, fs - 1.0f);
      }
      break;
    }
    case GUI_IMAGE:
      dc.texture(def.image.texId);
      dc.rectangle(ex + thm.border, ey + thm.border,
                   def.image.width, def.image.height,
                   def.image.texCoord0, def.image.texCoord1);
      break;
    case GUI_HFRAME:
    case GUI_VFRAME:
    case GUI_SPACER:
      break; // layout only - nothing to draw
    default:
      GX_LOG_ERROR("unknown type ", def.type);
      break;
  }

  // draw child elements
  if (!isPopup(def.type)) {
    for (auto& e : def.elems) {
      needRedraw |= drawElem(p, e, dc, dc2, usec, style);
    }
  }
  return needRedraw;
}

bool Gui::drawPopup(const Panel& p, GuiElem& def, DrawContext& dc,
                    DrawContext& dc2, int64_t usec) const
{
  bool needRedraw = false; // use for anim trigger later
  if (isPopup(def.type)) {
    // menu/listselect frame & items
    if (def._active) {
      const GuiTheme& thm = *p.theme;
      GuiElem& e1 = def.elems[1];
      needRedraw |= drawElem(
        p, e1, dc, dc2, usec,
        isMenu(def.type) ? &thm.menuFrame : &thm.listSelectFrame);
      // continue popup draw for possible active sub-menu
      needRedraw |= drawPopup(p, e1, dc, dc2, usec);
    }
  } else {
    for (auto& e : def.elems) { needRedraw |= drawPopup(p, e, dc, dc2, usec); }
  }
  return needRedraw;
}

GuiElem* Gui::findElemByID(ElemID id)
{
  for (auto& pPtr : _panels) {
    GuiElem* e = findElemByIDT(&pPtr->root, id);
    if (e) { return e; }
  }
  return nullptr;
}

GuiElem* Gui::findElemByEventID(EventID eid)
{
  for (auto& pPtr : _panels) {
    GuiElem* e = findElemByEventIDT(&pPtr->root, eid);
    if (e) { return e; }
  }
  return nullptr;
}

const GuiElem* Gui::findElemByEventID(EventID eid) const
{
  for (auto& pPtr : _panels) {
    GuiElem* e = findElemByEventIDT(&pPtr->root, eid);
    if (e) { return e; }
  }
  return nullptr;
}

GuiElem* Gui::findNextElem(ElemID id, GuiElemType type)
{
  Panel* p = nullptr;
  GuiElem* focusElem = nullptr;
  for (auto& pPtr : _panels) {
    p = pPtr.get();
    focusElem = findElemByIDT(&pPtr->root, id);
    if (focusElem) { break; }
  }

  if (!focusElem) { return nullptr; }
  const EventID eid = focusElem->eid;

  std::vector<GuiElem*> stack;
  GuiElem* e = &p->root;
  GuiElem* next = nullptr;
  GuiElem* first = nullptr;
  for (;;) {
    if (e->eid > 0 && e->_enabled && (type == GUI_NULL || e->type == type)) {
      if (e->eid == (eid+1)) { return e; }
      if (e->eid > eid && (!next || e->eid < next->eid)) { next = e; }
      if (!first || e->eid < first->eid) { first = e; }
    }

    if (!e->elems.empty()) {
      stack.reserve(stack.size() + e->elems.size());
      for (GuiElem& c : e->elems) { stack.push_back(&c); }
    }

    if (stack.empty()) { return next ? next : first; }
    e = stack.back();
    stack.pop_back();
  }
}

GuiElem* Gui::findPrevElem(ElemID id, GuiElemType type)
{
  Panel* p = nullptr;
  GuiElem* focusElem = nullptr;
  for (auto& pPtr : _panels) {
    p = pPtr.get();
    focusElem = findElemByIDT(&pPtr->root, id);
    if (focusElem) { break; }
  }

  if (!focusElem) { return nullptr; }
  const EventID eid = focusElem->eid;

  std::vector<GuiElem*> stack;
  GuiElem* e = &p->root;
  GuiElem* prev = nullptr;
  GuiElem* last = nullptr;
  for (;;) {
    if (e->eid > 0 && e->_enabled && (type == GUI_NULL || e->type == type)) {
      if (e->eid == (eid-1)) { return e; }
      if (e->eid < eid && (!prev || e->eid > prev->eid)) { prev = e; }
      if (!last || e->eid > last->eid) { last = e; }
    }

    if (!e->elems.empty()) {
      stack.reserve(stack.size() + e->elems.size());
      for (GuiElem& c : e->elems) { stack.push_back(&c); }
    }

    if (stack.empty()) { return prev ? prev : last; }
    e = stack.back();
    stack.pop_back();
  }
}
