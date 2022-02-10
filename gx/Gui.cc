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
  return isMenu(type) || (type == GUI_LISTSELECT);
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
  for (GuiElem& e : def.elems) { deactivate(e); }
}

static bool activate(GuiElem& def, ElemID id)
{
  if (def._id == id) {
    def._active = true;
  } else {
    // activate parent if child is activated to handle nested menus
    for (GuiElem& e : def.elems) { def._active |= activate(e, id); }
  }
  return def._active;
}

static int allElemState(GuiElem& def, bool enable)
{
  int count = 0;
  if (def.eid != 0) { def._enabled = enable; ++count; }
  for (GuiElem& e : def.elems) { count += allElemState(e, enable); }
  return count;
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
      // special case for listselect to prevent other listselect activation
      if (popupType == GUI_LISTSELECT && def.type == GUI_LISTSELECT
          && !def._active) { return nullptr; }

      if (contains(def, x, y)) { return &def; }
      else if (def._active) {
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

template<class T>
[[nodiscard]] static inline T* findElemByIDT(T& def, ElemID id)
{
  std::vector<T*> stack;
  T* e = &def;
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
[[nodiscard]] static inline T* findElemByEventIDT(T& def, EventID eid)
{
  assert(eid != 0);
  std::vector<T*> stack;
  T* e = &def;
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

[[nodiscard]] static GuiElem* findItem(GuiElem& root, int no)
{
  // NOTE: skips root in search
  for (GuiElem& e : root.elems) {
    if (e.type == GUI_LISTSELECT_ITEM && (no == 0 || e.itemNo == no)) {
      return &e;
    } else if (!e.elems.empty()) {
      GuiElem* e2 = findItem(e, no);
      if (e2) { return e2; }
    }
  }
  return nullptr;
}

[[nodiscard]] static GuiElem* findParentListSelect(GuiElem& root, ElemID id)
{
  // NOTE: skips root in search
  for (GuiElem& e : root.elems) {
    if (e.type == GUI_LISTSELECT && findElemByIDT(e.elems[1], id)) {
      return &e;
    } else if (!isMenu(e.type) && !e.elems.empty()) {
      GuiElem* e2 = findParentListSelect(e, id);
      if (e2) { return e2; }
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
                    const GuiTheme& thm, const GuiTheme::Style* style)
{
  if (style->backgroundType != GuiTheme::BG_NONE) {
    switch (style->backgroundType) {
      default: // BG_SOLID
        dc.color(style->backgroundColor);
        break;
      case GuiTheme::BG_VGRADIENT:
        dc.vgradient(y, style->backgroundColor, y+h, style->backgroundColor2);
        break;
    }

    switch (style->shapeType) {
      default: // SHAPE_DEFAULT
        dc.rectangle(x, y, w, h);
        break;
      case GuiTheme::SHAPE_ROUNDED: {
        dc.roundedRectangle(x, y, w, h, thm.roundedRadius, thm.roundedSegments);
        break;
      }
    }
  }

  if (style->edgeType != GuiTheme::EDGE_NONE && style->edgeColor != 0) {
    dc.color(style->edgeColor);
    switch (style->shapeType) {
      default: // SHAPE_DEFAULT
        switch (style->edgeType) {
          default: // EDGE_BORDER_1px
            dc.border(x, y, w, h, 1);
            break;
          case GuiTheme::EDGE_BORDER_2px:
            dc.border(x, y, w, h, 2);
            break;
          case GuiTheme::EDGE_UNDERLINE_1px:
            dc.rectangle(x, y + h - 1, w, 1);
            break;
          case GuiTheme::EDGE_UNDERLINE_2px:
            dc.rectangle(x, y + h - 2, w, 2);
            break;
          case GuiTheme::EDGE_OVERLINE_1px:
            dc.rectangle(x, y, w, 1);
            break;
          case GuiTheme::EDGE_OVERLINE_2px:
            dc.rectangle(x, y, w, 2);
            break;
        }
        break;
      case GuiTheme::SHAPE_ROUNDED:
        switch (style->edgeType) {
          default: // EDGE_BORDER_1px
            dc.roundedBorder(
              x, y, w, h, thm.roundedRadius, thm.roundedSegments, 1);
            break;
          case GuiTheme::EDGE_BORDER_2px:
            dc.roundedBorder(
              x, y, w, h, thm.roundedRadius, thm.roundedSegments, 2);
            break;
          case GuiTheme::EDGE_UNDERLINE_1px:
            dc.rectangle(
              x+thm.roundedRadius, y+h-1, w-(thm.roundedRadius*2), 1);
            break;
          case GuiTheme::EDGE_UNDERLINE_2px:
            dc.rectangle(
              x+thm.roundedRadius, y+h-2, w-(thm.roundedRadius*2), 2);
            break;
          case GuiTheme::EDGE_OVERLINE_1px:
            dc.rectangle(
              x+thm.roundedRadius, y, w-(thm.roundedRadius*2), 1);
            break;
          case GuiTheme::EDGE_OVERLINE_2px:
            dc.rectangle(
              x+thm.roundedRadius, y, w-(thm.roundedRadius*2), 2);
            break;
        }
        break;
    }
  }
}

[[nodiscard]] static constexpr uint16_t borderVal(
  const GuiTheme& thm, GuiElemType type)
{
  switch (type) {
    case GUI_PANEL:      return thm.panelBorder;
    case GUI_MENU_FRAME: return thm.menuFrameBorder;
    case GUI_VLINE:
    case GUI_HLINE:      return thm.lineBorder;
    default:             return thm.border;
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
        if (e.align & ALIGN_JUSTIFY) {
          if (e.align & ALIGN_HJUSTIFY) {
            e._w = def._w - (def.spacer.left + def.spacer.right);
          }
          if (e.align & ALIGN_VJUSTIFY) {
            e._h = def._h - (def.spacer.top + def.spacer.bottom);
          }
          resizedElem(thm, e);
        }
      }
      break;
    case GUI_PANEL:
    case GUI_MENU_FRAME: {
      GuiElem& e = def.elems[0];
      const float b2 = borderVal(thm, def.type) * 2;
      e._w = def._w - b2;
      e._h = def._h - b2;
      resizedElem(thm, e);
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
  // calculate child sizes before parent
  for (GuiElem& e : def.elems) { calcSize(thm, e); }

  switch (def.type) {
    case GUI_HFRAME: {
      float max_w = 0, max_h = 0;
      for (const GuiElem& e : def.elems) {
        max_w = std::max(max_w, e._w);
        max_h = std::max(max_h, e._h);
      }
      float total_w = -thm.frameSpacing;
      for (GuiElem& e : def.elems) {
        if (e.align & ALIGN_JUSTIFY) {
          if (e.align & ALIGN_HJUSTIFY) { e._w = max_w; }
          if (e.align & ALIGN_VJUSTIFY) { e._h = max_h; }
          resizedElem(thm, e);
        }
        total_w += e._w + thm.frameSpacing;
      }
      def._w = total_w;
      def._h = max_h;
      break;
    }
    case GUI_VFRAME: {
      float max_w = 0, max_h = 0;
      for (const GuiElem& e : def.elems) {
        max_w = std::max(max_w, e._w);
        max_h = std::max(max_h, e._h);
      }
      float total_h = -thm.frameSpacing;
      for (GuiElem& e : def.elems) {
        if (e.align & ALIGN_JUSTIFY) {
          if (e.align & ALIGN_HJUSTIFY) { e._w = max_w; }
          if (e.align & ALIGN_VJUSTIFY) { e._h = max_h; }
          resizedElem(thm, e);
        }
        total_h += e._h + thm.frameSpacing;
      }
      def._w = max_w;
      def._h = total_h;
      break;
    }
    case GUI_SPACER:
      def._w = def.spacer.left + def.spacer.right;
      def._h = def.spacer.top + def.spacer.bottom;
      if (!def.elems.empty()) {
        const GuiElem& e = def.elems[0];
        def._w += e._w;
        def._h += e._h;
      }
      break;
    case GUI_LABEL: {
      const Font& fnt = *thm.font;
      const int lines = calcLines(def.text);
      def._w = fnt.calcMaxLength(def.text, 0);
      def._h = float((fnt.size() - 1) * lines
                     + (thm.textSpacing * std::max(lines - 1, 0)));
      break;
    }
    case GUI_VLABEL: {
      const Font& fnt = *thm.font;
      const int lines = calcLines(def.text);
      def._w = float((fnt.size() - 1) * lines
                     + (thm.textSpacing * std::max(lines - 1, 0)));
      def._h = fnt.calcMaxLength(def.text, 0);
      break;
    }
    case GUI_HLINE:
      def._w = float(thm.font->size() - 1);
      def._h = float(thm.lineWidth + (thm.lineBorder * 2));
      break;
    case GUI_VLINE:
      def._w = float(thm.lineWidth + (thm.lineBorder * 2));
      def._h = float(thm.font->size() - 1);
      break;
    case GUI_CHECKBOX: {
      const GuiElem& e = def.elems[0];
      const Font& fnt = *thm.font;
      def._w = fnt.glyphWidth(thm.checkCode) + (thm.border * 3) + e._w;
      def._h = std::max(float(fnt.size() - 1 + (thm.border * 2)), e._h);
      break;
    }
    case GUI_SUBMENU: {
      // menu header
      const GuiElem& e = def.elems[0];
      def._w = e._w + (thm.border * 3) + thm.font->glyphWidth(thm.subMenuCode);
      def._h = e._h + (thm.border * 2);
      break;
    }
    case GUI_LISTSELECT: {
      // base size on 1st list item (all items should be same size)
      GuiElem& e1 = def.elems[1]; // GUI_MENU_FRAME
      const GuiElem* item = findItem(e1.elems[0], 0);
      assert(item != nullptr);
      def._w = item->_w;
      def._h = item->_h;
      break;
    }
    case GUI_LISTSELECT_ITEM: {
      const GuiElem& e = def.elems[0];
      def._w = e._w + (thm.border * 3)
        + std::max(thm.font->glyphWidth(thm.listSelectCode),
                   thm.font->glyphWidth(thm.listSelectOpenCode));
      def._h = e._h + (thm.border * 2);
      break;
    }
    case GUI_ENTRY: {
      const Font& fnt = *thm.font;
      if (def.entry.type == ENTRY_CARDINAL
          || def.entry.type == ENTRY_INTEGER
          || def.entry.type == ENTRY_FLOAT) {
        def._w = def.entry.size * fnt.digitWidth();
      } else {
        def._w = def.entry.size * fnt.glyphWidth('A');
        // FIXME: use better width value than capital A * size
      }
      def._w += float(thm.entryLeftMargin + thm.entryRightMargin
                      + thm.cursorWidth + 1);
      def._h = float(fnt.size() - 1)
        + thm.entryTopMargin + thm.entryBottomMargin;
      break;
    }
    case GUI_IMAGE: {
      const float b2 = thm.border * 2;
      def._w = def.image.width + b2;
      def._h = def.image.height + b2;
      break;
    }
    default:
      if (def.elems.empty()) {
        def._w = thm.emptyWidth;
        def._h = thm.emptyHeight;
      } else {
        const GuiElem& e = def.elems[0];
        const float b2 = borderVal(thm, def.type) * 2;
        def._w = e._w + b2;
        def._h = e._h + b2;
      }
      break;
  }
}

static void calcPos(const GuiTheme& thm, GuiElem& def,
                    float left, float top, float right, float bottom)
{
  switch (HAlign(def.align)) {
    case ALIGN_LEFT:  def._x = left; break;
    case ALIGN_RIGHT: def._x = right - def._w; break;
    default: // hcenter
      def._x = std::floor((left + right - def._w) * .5f); break;
  }

  switch (VAlign(def.align)) {
    case ALIGN_TOP:    def._y = top; break;
    case ALIGN_BOTTOM: def._y = bottom - def._h; break;
    default: // vcenter
      def._y = std::floor((top + bottom - def._h) * .5f + .5f); break;
  }

  left   = def._x;
  top    = def._y;
  right  = def._x + def._w;
  bottom = def._y + def._h;

  switch (def.type) {
    case GUI_HFRAME: {
      const float fs = thm.frameSpacing;
      float total_w = 0;
      for (const GuiElem& e : def.elems) { total_w += e._w + fs; }
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
      for (const GuiElem& e : def.elems) { total_h += e._h + fs; }
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
    case GUI_CHECKBOX:
      left += thm.font->glyphWidth(thm.checkCode) + (thm.border * 3);
      calcPos(thm, def.elems[0], left, top, right, bottom);
      break;
    case GUI_MENU: {
      GuiElem& e0 = def.elems[0];
      calcPos(thm, e0, left, top, right, bottom);
      // always position menu frame below menu button for now
      top = e0._y + e0._h + thm.border;
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
      if (!def.elems.empty()) {
        // align single child element
        assert(def.elems.size() == 1);
        const float b = borderVal(thm, def.type);
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


// **** Gui class ****
void Gui::clear()
{
  _panels.clear();
  clearHeld();
  _hoverID = 0;
  _focusID = 0;
  _popupID = 0;
  _eventID = 0;
  _popupType = GUI_NULL;
  _eventType = GUI_NULL;
  _eventTime = 0;
  _needRender = true;
  _textChanged = false;
}

void Gui::deletePanel(PanelID id)
{
  _needRender |= (removePanel(id) != nullptr);
}

void Gui::raisePanel(PanelID id)
{
  if (id == topPanel()) { return; }
  if (auto ptr = removePanel(id); ptr != nullptr) {
    _panels.insert(_panels.begin(), std::move(ptr));
    _needRender = true;
  }
}

void Gui::lowerPanel(PanelID id)
{
  if (id == bottomPanel()) { return; }
  if (auto ptr = removePanel(id); ptr != nullptr) {
    _panels.insert(_panels.end(), std::move(ptr));
    _needRender = true;
  }
}

bool Gui::getPanelLayout(PanelID id, Rect& layout) const
{
  const Panel* p = nullptr;
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
    if (win.allEvents() & (EVENT_MOUSE_MOVE | EVENT_MOUSE_ANY_BUTTON)) {
      processMouseEvent(win);
    }

    if (_eventID == 0 && _heldID != 0 && _repeatDelay >= 0
        && (now - _heldTime) > _repeatDelay) {
      if (_repeatDelay > 0) {
        _heldTime += _repeatDelay * ((now - _heldTime) / _repeatDelay);
      }

      const GuiElem* heldElem = findElemByID(_heldID);
      if (heldElem) { setEvent(*heldElem, now); }
    }
  } else if (_popupID != 0) {
    deactivatePopups();
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
  _popupType = GUI_NULL;
  _needRender = true;
}

void Gui::activatePopup(const GuiElem& def)
{
  if (_popupID != 0) { deactivatePopups(); }
  const ElemID id = def._id;
  for (auto& pPtr : _panels) { if (activate(pPtr->root, id)) break; }
  _popupID = id;
  _popupType = getPopupType(def.type);
  _needRender = true;
}

void Gui::processMouseEvent(Window& win)
{
  const bool lbuttonDown = win.buttons() & BUTTON1;
  const bool rbuttonDown = win.buttons() & BUTTON2;
  const bool lbuttonEvent = win.events() & EVENT_MOUSE_BUTTON1;
  const bool rbuttonEvent = win.events() & EVENT_MOUSE_BUTTON2;
  const bool lpressEvent = lbuttonDown & lbuttonEvent;
  const bool rpressEvent = rbuttonDown & rbuttonEvent;
  const bool anyGuiButtonEvent = win.allEvents() & EVENT_MOUSE_ANY_BUTTON;

  // get elem at mouse pointer
  MouseShapeEnum shape = MOUSESHAPE_ARROW;
  Panel* pPtr = nullptr;
  GuiElem* ePtr = nullptr;
  ElemID id = 0;
  GuiElemType type = GUI_NULL;
  if (win.mouseIn()) {
    for (auto& ptr : _panels) {
      const Panel& p = *ptr;
      const float mx = win.mouseX() - p.layout.x;
      const float my = win.mouseY() - p.layout.y;
      ePtr = findElemByXY(ptr->root, mx, my, _popupType);
      if (ePtr) {
        if (ePtr->_enabled) {
          id = ePtr->_id;
          type = ePtr->type;
          pPtr = ptr.get();
          if (type == GUI_ENTRY) { shape = MOUSESHAPE_IBEAM; }
        }

        win.removeEvent(EVENT_MOUSE_ANY_BUTTON);
        break;
      }
    }
  }

  if (shape != win.mouseShape()) {
    win.setMouseShape(shape);
  }

  // update focus
  // FIXME: setFocusID() could trigger an event that is overridden below
  if (lpressEvent) {
    setFocusID(win, (type == GUI_ENTRY) ? id : 0);
  } else if (lbuttonDown && anyGuiButtonEvent) {
    // click in other Gui instance clears our focus
    setFocusID(win, 0);
  }

  // update hoverID
  if (_hoverID != id) {
    const ElemID hid =
      (!lbuttonDown || isPopupItem(type) || id == _heldID) ? id : 0;
    if (_hoverID != hid) {
      _hoverID = hid;
      _needRender = true;
    }
  }

  if (lbuttonDown && _heldType == GUI_TITLEBAR) {
    if (win.events() & EVENT_MOUSE_MOVE) {
      pPtr = nullptr;
      for (auto& ptr : _panels) {
        if (findElemByIDT(ptr->root, _heldID)) { pPtr = ptr.get(); break; }
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
    const bool pressEvent = lpressEvent || (type == GUI_MENU && rpressEvent);
    if (pressEvent && ePtr->_active) {
      // click on open menu/listselect button closes popup
      deactivatePopups();
    } else if (pressEvent || (isMenu(type) && _popupType == GUI_MENU)) {
      // open menu/listselect with click OR open menu/sub-menu with mouse-over
      if (_popupID != id) { activatePopup(*ePtr); }
    }
  } else if (type == GUI_MENU_ITEM) {
    if (lbuttonEvent || rbuttonEvent) {
      setEvent(*ePtr, win.lastPollTime());
      deactivatePopups();
    } else if (_popupID != id) {
      // activate on menu item to close sub-menus if necessary
      activatePopup(*ePtr);
    }
  } else if (type == GUI_LISTSELECT_ITEM) {
    if (lbuttonEvent) {
      GuiElem* parent = findParentListSelect(pPtr->root, id);
      if (parent) {
        GuiElem& ls = *parent;
        ls.itemNo = ePtr->itemNo;
        GuiElem& e0 = ls.elems[0];
        e0 = ePtr->elems[0];
        const float b = pPtr->theme->border;
        calcPos(*pPtr->theme, e0, ls._x + b, ls._y + b, ls._x + ls._w - b,
                ls._y + ls._h - b);
        setEvent(ls, win.lastPollTime());
        deactivatePopups();
      }
    }
  } else {
    if (lpressEvent && id != 0) {
      _heldID = id;
      _heldType = type;
      _heldTime = win.lastPollTime();
      _heldX = win.mouseX();
      _heldY = win.mouseY();
      _needRender = true;
      if (type == GUI_BUTTON_PRESS) {
        _repeatDelay = ePtr->button.repeatDelay;
        setEvent(*ePtr, win.lastPollTime());
      }
    } else if ((_heldType == GUI_BUTTON_PRESS) && (_heldID != id)) {
      // clear hold if cursor moves off BUTTON_PRESS
      clearHeld();
      _needRender = true;
    } else if (!lbuttonDown && (_heldID != 0)) {
      if ((type == GUI_BUTTON || type == GUI_CHECKBOX)
          && lbuttonEvent && (_heldID == id)) {
        // activate if cursor is over element & button is released
        setEvent(*ePtr, win.lastPollTime());
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
    if (focusElem) { setEvent(*focusElem, win.lastPollTime()); }
  }

  _focusID = id;
  if (id != 0) {
    _lastCursorUpdate = win.lastPollTime();
    const Panel* p = nullptr;
    for (auto& pPtr : _panels) {
      if (findElemByIDT(pPtr->root, _focusID)) { p = pPtr.get(); break; }
    }

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

void Gui::setAllElemState(PanelID id, bool enable)
{
  int count = 0;
  for (auto& pPtr : _panels) {
    Panel& p = *pPtr;
    if (id == 0 || p.id == id) { count += allElemState(p.root, enable); }
  }
  _needRender |= (count > 0);
}

bool Gui::setText(EventID eid, std::string_view text)
{
  for (auto& pPtr : _panels) {
    GuiElem* e = findElemByEventIDT(pPtr->root, eid);
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
    GuiElem* e = findElemByEventIDT(pPtr->root, eid);
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
    GuiElem* e = findElemByEventIDT(pPtr->root, eid);
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
  def._id = ++_lastElemID;
  if (def.type == GUI_LISTSELECT) {
    const GuiElem* e = findItem(def, def.itemNo);
    if (!e && def.itemNo != 0) { e = findItem(def, 0); }
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
  Panel& p, GuiElem& def, DrawContext& dc, DrawContext& dc2,
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
      drawRec(dc, ex, ey, ew, eh, thm, style);
      break;
    case GUI_TITLEBAR:
      style = &thm.titlebar;
      drawRec(dc, ex, ey, ew, eh, thm, style);
      break;
    case GUI_LABEL:
      assert(style != nullptr);
      dc2.color(style->textColor);
      dc2.text(TextFormatting{thm.font, float(thm.textSpacing)},
               ex, ey, ALIGN_TOP_LEFT, def.text);
      break;
    case GUI_VLABEL:
      assert(style != nullptr);
      dc2.color(style->textColor);
      dc2.text(TextFormatting{thm.font, float(thm.textSpacing), 0,
          Vec2(0,-1), Vec2(1,0), Vec2(0,-1), Vec2(1,0)},
        ex, ey+eh, ALIGN_TOP_LEFT, def.text);
      break;
    case GUI_HLINE: {
      const float b = thm.lineBorder;
      assert(style != nullptr);
      dc.color(style->textColor);
      dc.rectangle(ex, ey + b, ew, eh - (b*2));
      break;
    }
    case GUI_VLINE: {
      const float b = thm.lineBorder;
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
      drawRec(dc, ex, ey, ew, eh, thm, style);
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
      const float cw = thm.font->glyphWidth(thm.checkCode) + (b*2.0f);
      const float ch = float(thm.font->size() - 1) + (b*2.0f);
      drawRec(dc, ex, ey, cw, ch, thm, style);
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
      drawRec(dc, ex, ey, ew, eh, thm, style);
      needRedraw |= drawElem(p, def.elems[0], dc, dc2, usec, style);
      break;
    case GUI_MENU_ITEM:
      if (!def._enabled) {
        style = &thm.menuItemDisable;
      } else if (def._id == _hoverID) {
        style = &thm.menuItemSelect;
        drawRec(dc, ex, ey, ew, eh, thm, style);
      }
      break;
    case GUI_SUBMENU: {
      if (def._active) {
        style = &thm.menuItemSelect;
        drawRec(dc, ex, ey, ew, eh, thm, style);
      }
      needRedraw |= drawElem(p, def.elems[0], dc, dc2, usec, style);
      const float b = thm.border;
      dc2.color(style->textColor);
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
      drawRec(dc, ex, ey, ew, eh, thm, style);
      needRedraw |= drawElem(p, def.elems[0], dc, dc2, usec, style);
      const float b = thm.border;
      const int32_t code = def._active ?
        thm.listSelectOpenCode : thm.listSelectCode;
      dc2.color(style->textColor);
      dc2.glyph(TextFormatting{thm.font, 0}, ex + ew - b, ey + b,
                ALIGN_TOP_RIGHT, code);
      break;
    }
    case GUI_LISTSELECT_ITEM:
      if (!def._enabled) {
        style = &thm.listSelectItemDisable;
      } else if (def._id == _hoverID) {
        style = &thm.listSelectItemSelect;
        drawRec(dc, ex, ey, ew, eh, thm, style);
      }

      if (thm.listSelectItemCode != 0) {
        const GuiElem* parent = findParentListSelect(p.root, def._id);
        if (parent && parent->itemNo == def.itemNo) {
          const float b = thm.border;
          dc2.color(style->textColor);
          dc2.glyph(TextFormatting{thm.font, 0}, ex + ew - b, ey + b,
                    ALIGN_TOP_RIGHT, thm.listSelectItemCode);
        }
      }
      break;
    case GUI_ENTRY: {
      if (!def._enabled) {
        style = &thm.entryDisable;
      } else {
        style = (def._id == _focusID) ? &thm.entryFocus : &thm.entry;
      }
      drawRec(dc, ex, ey, ew, eh, thm, style);
      const std::string txt = (def.entry.type == ENTRY_PASSWORD)
        ? passwordStr(thm.passwordCode, def.text.size()) : def.text;
      const RGBA8 textColor = style->textColor;
      const float cw = thm.cursorWidth;
      const float tw = thm.font->calcLength(txt, 0);
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
    for (GuiElem& e : def.elems) {
      needRedraw |= drawElem(p, e, dc, dc2, usec, style);
    }
  }
  return needRedraw;
}

bool Gui::drawPopup(Panel& p, GuiElem& def, DrawContext& dc,
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
    for (GuiElem& e : def.elems) {
      needRedraw |= drawPopup(p, e, dc, dc2, usec);
    }
  }
  return needRedraw;
}

GuiElem* Gui::findElemByID(ElemID id)
{
  for (auto& pPtr : _panels) {
    GuiElem* e = findElemByIDT(pPtr->root, id);
    if (e) { return e; }
  }
  return nullptr;
}

GuiElem* Gui::findElemByEventID(EventID eid)
{
  for (auto& pPtr : _panels) {
    GuiElem* e = findElemByEventIDT(pPtr->root, eid);
    if (e) { return e; }
  }
  return nullptr;
}

const GuiElem* Gui::findElemByEventID(EventID eid) const
{
  for (auto& pPtr : _panels) {
    const GuiElem* e = findElemByEventIDT(pPtr->root, eid);
    if (e) { return e; }
  }
  return nullptr;
}

GuiElem* Gui::findNextElem(ElemID id, GuiElemType type)
{
  Panel* p = nullptr;
  const GuiElem* focusElem = nullptr;
  for (auto& pPtr : _panels) {
    p = pPtr.get();
    focusElem = findElemByIDT(pPtr->root, id);
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
  const GuiElem* focusElem = nullptr;
  for (auto& pPtr : _panels) {
    p = pPtr.get();
    focusElem = findElemByIDT(pPtr->root, id);
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
