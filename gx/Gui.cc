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
#include "Assert.hh"
#include <algorithm>
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

[[nodiscard]] static GuiElem* findElemByXY(
  GuiElem& def, float x, float y, GuiElemType popupType)
{
  if (isPopup(def.type)) {
    if (popupType == GUI_NULL || (getPopupType(def.type) == popupType)) {
      // special case for listselect to prevent other listselect activation
      if (popupType == GUI_LISTSELECT && def.type == GUI_LISTSELECT
          && !def._active) { return nullptr; }

      if (def.contains(x, y)) { return &def; }
      else if (def._active) {
        GuiElem* e = findElemByXY(def.elems[1], x, y, GUI_NULL);
        if (e) { return e; }
      }
    }
  } else if ((def.eid != 0 || def.type == GUI_LISTSELECT_ITEM
              || def.type == GUI_TITLEBAR) && popupType == GUI_NULL
             && def.contains(x, y)) {
    return &def;
  } else {
    for (GuiElem& c : def.elems) {
      GuiElem* e = findElemByXY(c, x, y, popupType);
      if (e) { return e; }
    }

    if (def.type == GUI_PANEL && popupType == GUI_NULL
        && def.contains(x, y)) { return &def; }
  }
  return nullptr;
}

template<class T>
[[nodiscard]] static inline T* findByElemID(T& root, ElemID id)
{
  std::vector<T*> stack;
  T* e = &root;
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
[[nodiscard]] static inline T* findByEventID(T& root, EventID eid)
{
  GX_ASSERT(eid != 0);
  std::vector<T*> stack;
  T* e = &root;
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
    if (e.type == GUI_LISTSELECT_ITEM && (no == 0 || e.item().no == no)) {
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
    if (e.type == GUI_LISTSELECT && findByElemID(e.elems[1], id)) {
      return &e;
    } else if (!isMenu(e.type) && !e.elems.empty()) {
      GuiElem* e2 = findParentListSelect(e, id);
      if (e2) { return e2; }
    }
  }
  return nullptr;
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
    case GUI_PANEL: return thm.panelBorder;
    case GUI_POPUP: return thm.popupBorder;
    case GUI_VLINE:
    case GUI_HLINE: return thm.lineBorder;
    default:        return thm.border;
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
          const auto& spacer = def.spacer();
          if (e.align & ALIGN_HJUSTIFY) {
            e._w = def._w - (spacer.left + spacer.right);
          }
          if (e.align & ALIGN_VJUSTIFY) {
            e._h = def._h - (spacer.top + spacer.bottom);
          }
          resizedElem(thm, e);
        }
      }
      break;
    case GUI_PANEL:
    case GUI_POPUP: {
      GuiElem& e = def.elems[0];
      const float b2 = borderVal(thm, def.type) * 2;
      e._w = def._w - b2;
      e._h = def._h - b2;
      resizedElem(thm, e);
      break;
    }
    case GUI_LISTSELECT: {
      // listselect popup list width
      GuiElem& e1 = def.elems[1]; // GUI_POPUP
      e1._w = def._w + (thm.popupBorder * 2.0f);
      resizedElem(thm, e1);
      break;
    }
    default:
      break;
  }
}

static void calcSize(GuiElem& def, const GuiTheme& thm)
{
  // calculate child sizes before parent
  for (GuiElem& e : def.elems) { calcSize(e, thm); }

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
    case GUI_SPACER: {
      const auto& spacer = def.spacer();
      def._w = spacer.left + spacer.right;
      def._h = spacer.top + spacer.bottom;
      if (!def.elems.empty()) {
        const GuiElem& e = def.elems[0];
        def._w += e._w;
        def._h += e._h;
      }
      break;
    }
    case GUI_LABEL: {
      const Font& fnt = *thm.font;
      const auto& label = def.label();
      const int lines = std::max(calcLines(label.text), label.minLines);
      def._w = std::max(fnt.calcMaxLength(label.text, 0), label.minLength);
      def._h = float((fnt.size() - 1) * lines
                     + (thm.textSpacing * std::max(lines - 1, 0)));
      break;
    }
    case GUI_VLABEL: {
      const Font& fnt = *thm.font;
      const auto& label = def.label();
      const int lines = std::max(calcLines(label.text), label.minLines);
      def._w = float((fnt.size() - 1) * lines
                     + (thm.textSpacing * std::max(lines - 1, 0)));
      def._h = std::max(fnt.calcMaxLength(label.text, 0), label.minLength);
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
      GuiElem& e1 = def.elems[1]; // GUI_POPUP
      const GuiElem* item = findItem(e1.elems[0], 0);
      GX_ASSERT(item != nullptr);
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
      const auto& ep = def.entry();
      if (ep.type == ENTRY_CARDINAL || ep.type == ENTRY_INTEGER
          || ep.type == ENTRY_FLOAT) {
        def._w = ep.size * fnt.digitWidth();
      } else {
        def._w = ep.size * fnt.glyphWidth('A');
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
      const auto& image = def.image();
      def._w = image.width + b2;
      def._h = image.height + b2;
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

static void calcPos(GuiElem& def, const GuiTheme& thm,
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
        calcPos(e, thm, left, top, right - total_w, bottom);
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
        calcPos(e, thm, left, top, right, bottom - total_h);
        top = e._y + e._h + fs;
      }
      break;
    }
    case GUI_SPACER:
      if (!def.elems.empty()) {
        const auto& spacer = def.spacer();
        calcPos(def.elems[0], thm,
                left + spacer.left, top + spacer.top,
                right - spacer.right, bottom - spacer.bottom);
      }
      break;
    case GUI_CHECKBOX:
      left += thm.font->glyphWidth(thm.checkCode) + (thm.border * 3);
      calcPos(def.elems[0], thm, left, top, right, bottom);
      break;
    case GUI_MENU: {
      GuiElem& e0 = def.elems[0];
      calcPos(e0, thm, left, top, right, bottom);
      // always position menu frame below menu button for now
      top = e0._y + e0._h + thm.border;
      GuiElem& e1 = def.elems[1];
      calcPos(e1, thm, left, top, left + e1._w, top + e1._h);
      break;
    }
    case GUI_SUBMENU: {
      const float b = thm.border;
      calcPos(def.elems[0], thm, left + b, top + b, right - b, bottom - b);
      // sub-menu items
      left += def._w;
      GuiElem& e1 = def.elems[1];
      calcPos(e1, thm, left, top, left + e1._w, top + e1._h);
      break;
    }
    case GUI_LISTSELECT: {
      const float b = thm.border;
      calcPos(def.elems[0], thm, left + b, top + b, right - b, bottom - b);
      calcPos(def.elems[1], thm, left - thm.popupBorder, top + def._h,
              right, bottom);
      break;
    }
    default:
      if (!def.elems.empty()) {
        // align single child element
        GX_ASSERT(def.elems.size() == 1);
        const float b = borderVal(thm, def.type);
        calcPos(def.elems[0], thm, left + b, top + b, right - b, bottom - b);
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
  _lastElemID = 0;
  clearHeld();
  _hoverID = 0;
  _focusID = 0;
  _popupID = 0;
  _popupType = GUI_NULL;
  _eventID = 0;
  _eventType = GUI_NULL;
  _eventTime = 0;
  _eventID2 = 0;
  _eventType2 = GUI_NULL;
  _eventTime2 = 0;
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

bool Gui::update(Window& win)
{
  const int64_t now = win.lastPollTime();

  // make saved event active
  _eventID = _eventID2;
  _eventType = _eventType2;
  _eventTime = _eventTime2;
  _eventID2 = 0;
  _eventType2 = GUI_NULL;
  _eventTime2 = 0;
  _needRedraw = false;

  for (auto& pPtr : _panels) {
    Panel& p = *pPtr;
    // size & position update
    if (p.needLayout) {
      calcSize(p.root, *p.theme);
      calcPos(p.root, *p.theme, 0, 0, p.layout.w, p.layout.h);
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

      const GuiElem* heldElem = findElem(_heldID);
      if (heldElem) { addEvent(*heldElem, now); }
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
      _needRender |= drawElem(win, p, p.root, dc, dc2, &(p.theme->panel));

      if (!dc2.empty()) {
        dc.append(dc2);
        dc2.clear();
      }
    }

    if (_popupID != 0) {
      for (auto it = _panels.rbegin(), end = _panels.rend(); it != end; ++it) {
        Panel& p = **it;
        _needRender |= drawPopup(win, p, p.root, dc, dc2);

        if (!dc2.empty()) {
          dc.append(dc2);
          dc2.clear();
        }
      }
    }
    _needRedraw = true;
  }

  return _needRedraw;
}

void Gui::deactivatePopups()
{
  for (auto& pPtr : _panels) { deactivate(pPtr->root); }
  _popupID = 0;
  _popupType = GUI_NULL;
  _needRender = true;
}

void Gui::activatePopup(Panel& p, const GuiElem& def)
{
  if (_popupID != 0) { deactivatePopups(); }
  _needRender |= activate(p.root, def._id);
  _popupID = def._id;
  _popupType = getPopupType(def.type);
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
      Panel& p = *ptr;
      const float px = win.mouseX() - p.layout.x;
      const float py = win.mouseY() - p.layout.y;
      ePtr = findElemByXY(p.root, px, py, _popupType);
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
  if (lpressEvent) {
    if (type == GUI_ENTRY) {
      setFocus(win, ePtr);
      const GuiTheme& thm = *(pPtr->theme);
      const auto& entry = ePtr->entry();
      _cursorBlinkTime = thm.cursorBlinkTime;
      _focusCursorPos = lengthUTF8(
        thm.font->fitText(entry.text, win.mouseX() - entry.tx + 1));
    } else {
      setFocus(win, nullptr);
    }
  } else if (lbuttonDown && anyGuiButtonEvent) {
    // click in other Gui instance clears our focus
    setFocus(win, nullptr);
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
      const auto [p,e] = findPanelElem(_heldID);
      GX_ASSERT(p != nullptr);
      raisePanel(p->id);
      const float mx = std::clamp(win.mouseX(), 0.0f, float(win.width()));
      const float my = std::clamp(win.mouseY(), 0.0f, float(win.height()));
      p->layout.x += mx - _heldX;
      p->layout.y += my - _heldY;
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
      if (_popupID != id) { activatePopup(*pPtr, *ePtr); }
    }
  } else if (type == GUI_MENU_ITEM) {
    if (lbuttonEvent || rbuttonEvent) {
      addEvent(*ePtr, win.lastPollTime());
      deactivatePopups();
    } else if (_popupID != id) {
      // activate on menu item to close sub-menus if necessary
      activatePopup(*pPtr, *ePtr);
    }
  } else if (type == GUI_LISTSELECT_ITEM) {
    if (lbuttonEvent) {
      GuiElem* parent = findParentListSelect(pPtr->root, id);
      if (parent) {
        GuiElem& ls = *parent;
        ls.item().no = ePtr->item().no;
        GuiElem& e0 = ls.elems[0];
        const GuiElem& src = ePtr->elems[0];
        e0.label().text = src.label().text;
        e0.eid  = src.eid;
        const float b = pPtr->theme->border;
        calcPos(e0, *pPtr->theme, ls._x + b, ls._y + b, ls._x + ls._w - b,
                ls._y + ls._h - b);
        addEvent(ls, win.lastPollTime());
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
        _repeatDelay = ePtr->button().repeatDelay;
        addEvent(*ePtr, win.lastPollTime());
      }
    } else if ((_heldType == GUI_BUTTON_PRESS) && (_heldID != id)) {
      // clear hold if cursor moves off BUTTON_PRESS
      clearHeld();
      _needRender = true;
    } else if (!lbuttonDown && (_heldID != 0)) {
      if ((type == GUI_BUTTON || type == GUI_CHECKBOX)
          && lbuttonEvent && (_heldID == id)) {
        // activate if cursor is over element & button is released
        addEvent(*ePtr, win.lastPollTime());
        if (type == GUI_CHECKBOX) { ePtr->checkbox().set = !ePtr->checkbox().set; }
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
  GuiElem* e = findElem(_focusID);
  if (!e) { return; }
  GX_ASSERT(e->type == GUI_ENTRY);
  auto& entry = e->entry();

  bool usedEvent = false;
  for (const CharInfo& c : win.charData()) {
    if (c.codepoint) {
      usedEvent = true;
      if (addEntryChar(*e, int32_t(c.codepoint))) {
        _needRender = _textChanged = true;
      }
      // TODO: flash 'error' color if char isn't added
    } else if (c.key == KEY_BACKSPACE) {
      usedEvent = true;
      if (_focusCursorPos > 0) {
        GX_ASSERT(!entry.text.empty());
        if (c.mods == MOD_CONTROL) {
          entry.text.erase(0, indexUTF8(entry.text, _focusCursorPos));
          _focusCursorPos = 0;
        } else {
          eraseUTF8(entry.text, --_focusCursorPos);
        }
        _needRender = _textChanged = true;
      }
    } else if (c.key == KEY_DELETE) {
      usedEvent = true;
      if (_focusCursorPos < lengthUTF8(entry.text)) {
        if (c.mods == MOD_CONTROL) {
          entry.text.erase(indexUTF8(entry.text, _focusCursorPos), std::string::npos);
        } else {
          eraseUTF8(entry.text, _focusCursorPos);
        }
        _needRender = _textChanged = true;
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
      usedEvent = true;
      setFocus(win, findNextElem(_focusID, GUI_ENTRY));
    } else if (c.key == KEY_TAB && c.mods == MOD_SHIFT) {
      usedEvent = true;
      setFocus(win, findPrevElem(_focusID, GUI_ENTRY));
    } else if (c.key == KEY_LEFT && c.mods == 0) {
      usedEvent = true;
      if (_focusCursorPos > 0) { --_focusCursorPos; _needRender = true; }
    } else if (c.key == KEY_RIGHT && c.mods == 0) {
      usedEvent = true;
      if (_focusCursorPos < lengthUTF8(entry.text)) {
        ++_focusCursorPos; _needRender = true; }
    } else if (c.key == KEY_HOME && c.mods == 0) {
      usedEvent = true;
      if (_focusCursorPos > 0) { _focusCursorPos = 0; _needRender = true; }
    } else if (c.key == KEY_END && c.mods == 0) {
      usedEvent = true;
      const std::size_t ts = lengthUTF8(entry.text);
      if (_focusCursorPos < ts) { _focusCursorPos = ts; _needRender = true; }
    }
  }

  if (usedEvent) {
    win.removeEvent(EVENT_CHAR);
    // reset cursor blink state
    _needRender |= !_cursorState;
    _lastCursorUpdate = win.lastPollTime();
    _cursorState = true;
  }
}

bool Gui::addEntryChar(GuiElem& e, int32_t code)
{
  GX_ASSERT(e.type == GUI_ENTRY);
  auto& entry = e.entry();
  if (entry.maxLength != 0 && lengthUTF8(entry.text) >= entry.maxLength) {
    return false; // no space for character
  }

  switch (entry.type) {
    default: // ENTRY_TEXT, ENTRY_PASSWORD
      // valid character check
      if (code <= 31) { return false; }
      break;
    case ENTRY_CARDINAL:
      // valid character check
      if (!std::isdigit(code)) { return false; }
      // allowed sequence check
      if (code == '0'
          && (entry.text == "0" || (_focusCursorPos == 0 && !entry.text.empty()))) {
        return false; }
      // special case to reset entry
      if (entry.text == "0" && _focusCursorPos == 1) {
        entry.text.clear(); _focusCursorPos = 0; }
      break;
    case ENTRY_INTEGER:
      // valid character check
      if (!std::isdigit(code) && code != '-') { return false; }
      // allowed sequence check
      if ((code == '-' && !entry.text.empty() && entry.text != "0")
          || (code == '0' && (entry.text == "0" || entry.text == "-"))) { return false; }
      // special case to reset entry
      if (entry.text == "0" && _focusCursorPos == 1) {
        entry.text.clear(); _focusCursorPos = 0; }
      break;
    case ENTRY_FLOAT:
      // valid character check
      if (!std::isdigit(code) && code != '-' && code != '.') { return false; }
      // allowed sequence check
      if ((code == '-' && !entry.text.empty() && entry.text != "0")
          || (code == '0' && (entry.text == "0" || entry.text == "-0"))) {
        return false; }
      // decimal handling and special case to reset entry
      if (code == '.') {
        int count = 0;
        for (int ch : entry.text) { count += (ch == '.'); }
        if (count > 0) { return false; }
      } else if (entry.text == "0"  && _focusCursorPos == 1) {
        entry.text.clear(); _focusCursorPos = 0;
      }
      break;
  }

  insertUTF8(entry.text, _focusCursorPos, code);
  ++_focusCursorPos;
  return true;
}

void Gui::setFocus(Window& win, const GuiElem* e)
{
  // reset cursor blink
  _lastCursorUpdate = win.lastPollTime();
  _cursorState = true;

  const ElemID id = e ? e->_id : 0;
  if (_focusID == id) { return; }

  if (_textChanged) {
    _textChanged = false;
    GuiElem* focusElem = findElem(_focusID);
    if (focusElem) {
      auto& entry = focusElem->entry();
      if (entry.text.empty()) {
        const EntryType t = entry.type;
        if (t == ENTRY_CARDINAL || t == ENTRY_INTEGER || t == ENTRY_FLOAT) {
          entry.text = "0";
        }
      }
      addEvent(*focusElem, win.lastPollTime());
    }
  }

  _focusID = id;
  _focusCursorPos = e ? lengthUTF8(e->entry().text) : 0;
  _focusEntryOffset = 0;
  _needRender = true;
}

void Gui::setElemState(EventID eid, bool enable)
{
  GuiElem* e = findEventElem(eid);
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
    GuiElem* e = findByEventID(pPtr->root, eid);
    if (!e) { continue; }

    switch (e->type) {
      case GUI_ENTRY:
        e->entry().text = text;
        if (_focusID == e->_id) {
          _focusCursorPos = lengthUTF8(text);
          _focusEntryOffset = 0;
        }
        break;
      case GUI_LABEL:
      case GUI_VLABEL:
        e->label().text = text;
        pPtr->needLayout = true;
        break;

      default:
        return false;
    }

    _needRender = true;
    return true;
  }
  return false;
}

bool Gui::setBool(EventID eid, bool val)
{
  GuiElem* e = findEventElem(eid);
  if (!e || e->type != GUI_CHECKBOX) { return false; }

  e->checkbox().set = val;
  _needRender = true;
  return true;
}

bool Gui::setItemNo(EventID eid, int no)
{
  GuiElem* e = findEventElem(eid);
  if (!e || e->type != GUI_LISTSELECT) { return false; }

  e->item().no = no;
  _needRender = true;
  return true;
}

PanelID Gui::addPanel(PanelPtr ptr, float x, float y, AlignEnum align)
{
  initElem(ptr->root);
  layout(*ptr, x, y, align);

  if (ptr->id <= 0) {
    PanelID id = 0;
    for (auto& pPtr : _panels) { id = std::max(id, pPtr->id); }
    ptr->id = id + 1;
  } else {
    removePanel(ptr->id);
  }

  const auto itr = _panels.insert(_panels.begin(), std::move(ptr));
  return (*itr)->id;
}

void Gui::layout(Panel& p, float x, float y, AlignEnum align)
{
  const GuiTheme& thm = *p.theme;
  GX_ASSERT(thm.font != nullptr);
  p.layout = {x,y,0,0};
  if (HAlign(align) == ALIGN_RIGHT) { std::swap(p.layout.x, p.layout.w); }
  if (VAlign(align) == ALIGN_BOTTOM) { std::swap(p.layout.y, p.layout.h); }
  p.needLayout = false;
  p.root.align = align;
  calcSize(p.root, thm);
  calcPos(p.root, thm, 0, 0, p.layout.w, p.layout.h);
  _needRender = true;
}

void Gui::initElem(GuiElem& def)
{
  def._id = ++_lastElemID;
  if (def.type == GUI_LISTSELECT) {
    const GuiElem* e = findItem(def, def.item().no);
    if (!e && def.item().no != 0) { e = findItem(def, 0); }
    if (e) {
      GuiElem& e0 = def.elems[0];
      const GuiElem& src = e->elems[0];
      e0.label().text = src.label().text;
      e0.align = ALIGN_CENTER_LEFT;
      e0.eid   = src.eid;
      def.item().no = e->item().no;
    }
  }
  for (GuiElem& e : def.elems) { initElem(e); }
}

bool Gui::drawElem(
  Window& win, Panel& p, GuiElem& def, DrawContext& dc, DrawContext& dc2,
  const GuiTheme::Style* style)
{
  const GuiTheme& thm = *p.theme;
  switch (def.type) {
    case GUI_TITLEBAR:
      style = &thm.titlebar;
      break;
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
      break;
    case GUI_CHECKBOX:
      if (!def._enabled) {
        style = &thm.checkboxDisable;
      } else if (def._id == _heldID) {
        style = (def._id == _hoverID) ? &thm.checkboxPress : &thm.checkboxHold;
      } else {
        style = (def._id == _hoverID) ? &thm.checkboxHover : &thm.checkbox;
      }
      break;
    case GUI_MENU:
      style = def._active ? &thm.menuButtonOpen
        : ((def._id == _hoverID) ? &thm.menuButtonHover : &thm.menuButton);
      break;
    case GUI_SUBMENU:
      if (!def._enabled) {
        style = &thm.menuItemDisable;
      } else {
        style = def._active ? &thm.menuItemSelect : &thm.menuItem;
      }
      break;
    case GUI_MENU_ITEM:
      if (!def._enabled) {
        style = &thm.menuItemDisable;
      } else {
        style = (def._id == _hoverID) ? &thm.menuItemSelect : &thm.menuItem;
      }
      break;
    case GUI_LISTSELECT:
      if (!def._enabled) {
        style = &thm.listSelectDisable;
      } else {
        style = def._active ? &thm.listSelectOpen
          : ((def._id == _hoverID) ? &thm.listSelectHover : &thm.listSelect);
      }
      break;
    case GUI_LISTSELECT_ITEM:
      if (!def._enabled) {
        style = &thm.listSelectItemDisable;
      } else {
        style = (def._id == _hoverID)
          ? &thm.listSelectItemSelect : &thm.listSelectItem;
      }
      break;
    case GUI_ENTRY:
      if (!def._enabled) {
        style = &thm.entryDisable;
      } else {
        style = (def._id == _focusID) ? &thm.entryFocus : &thm.entry;
      }
      break;
    default:
      GX_ASSERT(style != nullptr);
      break;
  }

  const float ex = def._x + p.layout.x;
  const float ey = def._y + p.layout.y;
  const float ew = def._w;
  const float eh = def._h;

  bool needRedraw = false; // use for anim trigger later
  switch (def.type) {
    case GUI_LABEL:
      dc2.color(style->textColor);
      dc2.text({thm.font, float(thm.textSpacing)},
               ex, ey, ALIGN_TOP_LEFT, def.label().text);
      break;
    case GUI_VLABEL:
      dc2.color(style->textColor);
      dc2.text({thm.font, float(thm.textSpacing), 0,
                {0,-1}, {1,0}, {0,-1}, {1,0}},
               ex, ey+eh, ALIGN_TOP_LEFT, def.label().text);
      break;
    case GUI_HLINE: {
      const float b = thm.lineBorder;
      dc.color(style->textColor);
      dc.rectangle(ex, ey + b, ew, eh - (b*2));
      break;
    }
    case GUI_VLINE: {
      const float b = thm.lineBorder;
      dc.color(style->textColor);
      dc.rectangle(ex + b, ey, ew - (b*2), eh);
      break;
    }
    case GUI_CHECKBOX: {
      const float b = thm.border;
      const float cw = thm.font->glyphWidth(thm.checkCode) + (b*2);
      const float ch = float(thm.font->size() - 1) + (b*2);
      drawRec(dc, ex, ey, cw, ch, thm, style);
      if (def.checkbox().set) {
        dc2.color(style->textColor);
        dc2.glyph({thm.font, float(thm.textSpacing)},
                  ex + b + thm.checkXOffset, ey + b + thm.checkYOffset,
                  ALIGN_TOP_LEFT, thm.checkCode);
      }
      break;
    }
    case GUI_SUBMENU:
      drawRec(dc, ex, ey, ew, eh, thm, style);
      dc2.color(style->textColor);
      dc2.glyph({thm.font, float(thm.textSpacing)},
                ex + ew, ey + thm.border, ALIGN_TOP_RIGHT, thm.subMenuCode);
      break;
    case GUI_LISTSELECT: {
      drawRec(dc, ex, ey, ew, eh, thm, style);
      const float b = thm.border;
      const int32_t code = def._active
        ? thm.listSelectOpenCode : thm.listSelectCode;
      dc2.color(style->textColor);
      dc2.glyph({thm.font, float(thm.textSpacing)},
                ex + ew - b, ey + b, ALIGN_TOP_RIGHT, code);
      break;
    }
    case GUI_LISTSELECT_ITEM:
      drawRec(dc, ex, ey, ew, eh, thm, style);
      if (thm.listSelectItemCode != 0) {
        const GuiElem* parent = findParentListSelect(p.root, def._id);
        if (parent && parent->item().no == def.item().no) {
          const float b = thm.border;
          dc2.color(style->textColor);
          dc2.glyph({thm.font, float(thm.textSpacing)}, ex + ew - b, ey + b,
                    ALIGN_TOP_RIGHT, thm.listSelectItemCode);
        }
      }
      break;
    case GUI_ENTRY: {
      drawRec(dc, ex, ey, ew, eh, thm, style);
      auto& entry = def.entry();
      const std::string txt = (entry.type == ENTRY_PASSWORD)
        ? passwordStr(thm.passwordCode, lengthUTF8(entry.text)) : entry.text;
      const float cw = thm.cursorWidth;
      const float tw = thm.font->calcLength(txt, 0);
      const float maxWidth = ew - thm.entryLeftMargin
        - thm.entryRightMargin - cw;
      const float leftEdge = ex + thm.entryLeftMargin;
      const float rightEdge = leftEdge + maxWidth;
      float tx = leftEdge;
      if (tw <= maxWidth) {
        if (HAlign(entry.align) == ALIGN_RIGHT) {
          tx = ex + ew - (tw + cw + thm.entryRightMargin);
        } else if (HAlign(entry.align) != ALIGN_LEFT) { // HCENTER
          tx = ex + ((ew - tw) * .5f);
        }
      }
      float cx = 0;
      if (def._id == _focusID) {
        if (tw <= maxWidth) { _focusEntryOffset = 0; }
        cx = tx + _focusEntryOffset + thm.font->calcLength(
          {txt.c_str(), indexUTF8(txt,_focusCursorPos)}, 0);
        if (cx < leftEdge) {
          _focusEntryOffset += leftEdge - cx; cx = leftEdge;
        } else if (cx > rightEdge) {
          _focusEntryOffset -= cx - rightEdge; cx = rightEdge;
        } else if (tw > maxWidth) {
          // special case to keep max amount of entry text visible
          const float tEnd = tx + _focusEntryOffset + tw;
          if (tEnd < rightEdge) {
            _focusEntryOffset += rightEdge - tEnd;
            cx += rightEdge - tEnd;
          }
        }
        tx += _focusEntryOffset;
      }
      entry.tx = tx;
      dc2.color(style->textColor);
      // TODO: add gradient color if text is off left/right edges
      dc2.text({thm.font, float(thm.textSpacing)}, tx,
               ey + thm.entryTopMargin, ALIGN_TOP_LEFT, txt, {ex, ey, ew, eh});
      if (def._id == _focusID && _cursorState) {
        // draw cursor
        dc.color(thm.cursorColor);
        dc.rectangle(cx - 1, ey + thm.entryTopMargin,
                     cw, float(thm.font->size() - 1));
      }
      break;
    }
    case GUI_IMAGE: {
      const auto& image = def.image();
      dc.texture(image.texId);
      dc.rectangle(ex + thm.border, ey + thm.border,
                   image.width, image.height,
                   image.texCoord0, image.texCoord1);
      break;
    }
    case GUI_HFRAME:
    case GUI_VFRAME:
    case GUI_SPACER:
      break; // layout only - nothing to draw
    default:
      drawRec(dc, ex, ey, ew, eh, thm, style);
      break;
  }

  // draw child elements
  for (GuiElem& e : def.elems) {
    if (e.type != GUI_POPUP) {
      needRedraw |= drawElem(win, p, e, dc, dc2, style);
    }
  }
  return needRedraw;
}

bool Gui::drawPopup(Window& win, Panel& p, GuiElem& def,
                    DrawContext& dc, DrawContext& dc2)
{
  bool needRedraw = false; // use for anim trigger later
  for (GuiElem& e : def.elems) {
    if (def._active && e.type == GUI_POPUP) {
      const GuiTheme& thm = *p.theme;
      needRedraw |= drawElem(
        win, p, e, dc, dc2,
        isMenu(def.type) ? &thm.menuFrame : &thm.listSelectFrame);
    }
    needRedraw |= drawPopup(win, p, e, dc, dc2);
  }
  return needRedraw;
}

GuiElem* Gui::findElem(ElemID id)
{
  for (auto& pPtr : _panels) {
    GuiElem* e = findByElemID(pPtr->root, id);
    if (e) { return e; }
  }
  return nullptr;
}

std::pair<Gui::Panel*,GuiElem*> Gui::findPanelElem(ElemID id)
{
  for (auto& pPtr : _panels) {
    GuiElem* e = findByElemID(pPtr->root, id);
    if (e) { return {pPtr.get(),e}; }
  }
  return {nullptr,nullptr};
}

GuiElem* Gui::findEventElem(EventID eid)
{
  for (auto& pPtr : _panels) {
    GuiElem* e = findByEventID(pPtr->root, eid);
    if (e) { return e; }
  }
  return nullptr;
}

const GuiElem* Gui::findEventElem(EventID eid) const
{
  for (auto& pPtr : _panels) {
    const GuiElem* e = findByEventID(pPtr->root, eid);
    if (e) { return e; }
  }
  return nullptr;
}

GuiElem* Gui::findNextElem(ElemID id, GuiElemType type)
{
  const auto [p,focusElem] = findPanelElem(id);
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
  const auto [p,focusElem] = findPanelElem(id);
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

static bool buttonActionAdd(GuiElem& target, double value)
{
  if (target.type != GUI_ENTRY) return false;

  auto& entry = target.entry();
  switch (entry.type) {
    case ENTRY_INTEGER: {
      const int64_t eval = std::stoll(entry.text) + int64_t(value);
      entry.text = std::to_string(eval);
      break;
    }

    case ENTRY_CARDINAL: {
      const int64_t eval =
        std::max(0ll, std::stoll(entry.text) + int64_t(value));
      entry.text = std::to_string(eval);
      break;
    }

    case ENTRY_FLOAT: {
      const double eval = std::stof(entry.text) + value;
      entry.text = std::to_string(eval);
      break;
    }

    default:
      return false;
  }

  return true;
}

static bool buttonActionSet(GuiElem& target, double value)
{
  if (target.type != GUI_ENTRY) return false;

  auto& entry = target.entry();
  switch (entry.type) {
    case ENTRY_INTEGER:
      entry.text = std::to_string(int64_t(value));
      break;

    case ENTRY_CARDINAL:
      entry.text = std::to_string(std::max(int64_t{0}, int64_t(value)));
      break;

    case ENTRY_FLOAT:
      entry.text = std::to_string(value);
      break;

    default:
      return false;
  }

  return true;
}

void gx::Gui::addEvent(const GuiElem& e, int64_t t)
{
  GuiElem* target = nullptr;
  if (e.type == GUI_BUTTON || e.type == GUI_BUTTON_PRESS) {
    const GuiAction& action = e.button().action;
    if (action.type != GuiAction::NONE) {
      target = findEventElem(action.targetID);
      if (!target) {
        GX_LOG_ERROR("Unknown targetID ",action.targetID," for GuiAction");
        return;
      }

      bool update = false;
      switch (action.type) {
        case GuiAction::ADD:
          update |= buttonActionAdd(*target, action.value); break;
        case GuiAction::SET:
          update |= buttonActionSet(*target, action.value); break;
        default: break;
      }

      if (update) {
        _needRender = true;
        if (_focusID == target->_id) {
          _focusCursorPos = lengthUTF8(target->entry().text);
          _focusEntryOffset = 0;
        }
      }
    }
  }

  const GuiElem& eventElem = target ? *target : e;
  if (_eventID != 0) {
    // save event for next update
    _eventID2 = eventElem.eid;
    _eventType2 = eventElem.type;
    _eventTime2 = t;
  } else {
    _eventID = eventElem.eid;
    _eventType = eventElem.type;
    _eventTime = t;
  }
}

gx::Gui::PanelPtr gx::Gui::removePanel(PanelID id)
{
  for (auto i = _panels.begin(), end = _panels.end(); i != end; ++i) {
    if ((*i)->id == id) {
      auto ptr = std::move(*i);
      _panels.erase(i);
      return ptr;
    }
  }
  return {};
}
