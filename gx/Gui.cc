//
// gx/Gui.cc
// Copyright (C) 2021 Richard Bradley
//

// TODO: handle tab/enter/mouse select differently for entry
// TODO: cursor movement for entry
// TODO: allow right button to open menus & select menu items
// TODO: menu item key short-cuts

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


[[nodiscard]] static bool contains(const GuiElem& e, float x, float y)
{
  return (x >= e._x) && (x < (e._x + e._w))
    && (y >= e._y) && (y < (e._y + e._h));
}

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

static void deactivate(GuiElem& def)
{
  def._active = false;
  for (auto& e : def.elems) { deactivate(e); }
}

static bool activate(GuiElem& def, EventID id)
{
  if (def.id == id) {
    def._active = true;
  } else {
    // activate parent if child is activated to handle nested menus
    for (auto& e : def.elems) { def._active |= activate(e, id); }
  }
  return def._active;
}

[[nodiscard]]
static GuiElem* findElemByXY(GuiElem& def, float x, float y, bool popup)
{
  if (isMenu(def.type)) {
    if (contains(def, x, y)) { return &def; }
    if (def._active) {
      GuiElem* e = findElemByXY(def.elems[1], x, y, false);
      if (e) { return e; }
    }
  } else {
    if (!popup && def.id != 0 && contains(def, x, y)) { return &def; }
    for (GuiElem& c : def.elems) {
      GuiElem* e = findElemByXY(c, x, y, popup);
      if (e) { return e; }
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

static inline void drawRec(DrawContext& dc, const GuiElem& e,
                           const GuiTheme::Style* style)
{
  drawRec(dc, e._x, e._y, e._w, e._h, style);
}

static void calcSize(const GuiTheme& thm, GuiElem& def)
{
  const float b = thm.border;
  switch (def.type) {
    case GUI_HFRAME: {
      const float fs = thm.frameSpacing;
      float total_w = -fs, max_h = 0;
      for (GuiElem& e : def.elems) {
        calcSize(thm, e);
        total_w += e._w + fs;
        max_h = std::max(max_h, e._h);
      }
      for (GuiElem& e : def.elems) {
        if (e.align & ALIGN_VJUSTIFY) { e._h = max_h; }
        // TODO: support horizontal justify
      }
      def._w = total_w;
      def._h = max_h;
      break;
    }
    case GUI_VFRAME: {
      const float fs = thm.frameSpacing;
      float total_h = -fs, max_w = 0;
      for (GuiElem& e : def.elems) {
        calcSize(thm, e);
        total_h += e._h + fs;
        max_w = std::max(max_w, e._w);
      }
      for (GuiElem& e : def.elems) {
        if (e.align & ALIGN_HJUSTIFY) { e._w = max_w; }
        // TODO: support vertical justify
      }
      def._w = max_w;
      def._h = total_h;
      break;
    }
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
    case GUI_BACKGROUND:
    case GUI_BUTTON:
    case GUI_BUTTON_PRESS:
    case GUI_BUTTON_HOLD:
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
static inline T* findElemT(T* root, EventID id)
{
  assert(id != 0);
  std::vector<T*> stack;
  T* e = root;
  while (e->id != id) {
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
  _focusID = 0;
  _eventID = 0;
  _popupID = 0;
  _needRender = true;
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
  const Panel* p = findPanel(id);
  if (!p) { return false; }
  layout = p->layout;
  return true;
}

void Gui::update(Window& win)
{
  // clear event state that only persists for a single update
  _eventID = 0;
  _needRedraw = false;

  for (auto& pPtr : _panels) {
    Panel& p = *pPtr;
    // size & position update
    if (p.needLayout) {
      calcSize(*p.theme, p.root);
      calcPos(*p.theme, p.root, p.layout.x, p.layout.y,
              p.layout.x + p.layout.w, p.layout.y + p.layout.h);
      p.needLayout = false;
    }
  }

  // mouse movement/button handling
  if (win.focused()) {
    if (win.allEvents() & (EVENT_MOUSE_MOVE | EVENT_MOUSE_BUTTON1)) {
      processMouseEvent(win);
    }

    if (_heldType == GUI_BUTTON_HOLD && _eventID == 0) {
      _eventID = _heldID;
    }
  }

  // entry input handling & cursor update
  if (_focusID != 0) {
    if (win.events() & EVENT_CHAR) { processCharEvent(win); }

    if (_cursorBlinkTime > 0) {
      // check for cursor blink
      const int64_t blinks =
        (win.lastPollTime() - _lastCursorUpdate) / _cursorBlinkTime;
      if (blinks > 0) {
        _lastCursorUpdate += blinks * _cursorBlinkTime;
        if (blinks & 1) { _cursorState = !_cursorState; }
        _needRender = true;
      }
    }
  }

  // redraw GUI if needed
  if (_needRender) {
    DrawContext dc{_dl}, dc2{_dl2};
    dc.clear();
    dc2.clear();
    _needRender = false;
    const int64_t usec = win.lastPollTime();

    for (auto it = _panels.rbegin(), end = _panels.rend(); it != end; ++it) {
      Panel& p = **it;
      const GuiTheme& thm = *p.theme;
      _needRender |= drawElem(p.root, dc, dc2, usec, thm, &thm.panel);

      if (!dc2.empty()) {
        dc.append(dc2);
        dc2.clear();
      }
    }

    if (_popupID != 0) {
      for (auto it = _panels.rbegin(), end = _panels.rend(); it != end; ++it) {
        Panel& p = **it;
        _needRender |= drawPopup(p.root, dc, dc2, usec, *p.theme);

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

void Gui::activatePopup(EventID id)
{
  if (_popupID != 0) { deactivatePopups(); }
  for (auto& pPtr : _panels) { activate(pPtr->root, id); }
  _popupID = id;
  _needRender = true;
}

void Gui::processMouseEvent(Window& win)
{
  const bool buttonDown = win.buttons() & BUTTON1;
  const bool buttonEvent = win.events() & EVENT_MOUSE_BUTTON1;
  const bool pressEvent = buttonDown & buttonEvent;
  const bool anyGuiButtonEvent = win.allEvents() & EVENT_MOUSE_BUTTON1;

  // get elem at mouse pointer
  GuiElem* ePtr = nullptr;
  EventID id = 0;
  GuiElemType type = GUI_NULL;
  if (win.mouseIn()) {
    for (auto& pPtr : _panels) {
      ePtr = findElemByXY(pPtr->root, win.mouseX(), win.mouseY(), _popupID != 0);
      if (ePtr) {
        if (ePtr->_enabled) {
          id = ePtr->id;
          type = ePtr->type;
        }
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
    const EventID hid =
      (buttonDown && type != GUI_MENU_ITEM && id != _heldID) ? 0 : id;
    if (_hoverID != hid) {
      _hoverID = hid;
      _needRender = true;
    }
  }

  bool usedEvent = false;
  if (isMenu(type)) {
    if (pressEvent && ePtr->_active) {
      // click on open menu button closes it
      deactivatePopups();
      usedEvent = true;
    } else if (pressEvent || _popupID != 0) {
      // open menu
      if (_popupID != id) { activatePopup(id); }
      usedEvent = true;
    }
  } else if (type == GUI_MENU_ITEM) {
    // activate on menu item to close sub-menus if neccessary
    if (_popupID != id) { activatePopup(id); }
    if (buttonEvent) {
      _eventID = id;
      usedEvent = true;
    }
  } else {
    if (pressEvent && id != 0) {
      _heldID = id;
      _heldType = type;
      _needRender = true;
      usedEvent = true;
      if (type == GUI_BUTTON_PRESS) { _eventID = id; }
    }

    if ((_heldType == GUI_BUTTON_PRESS || _heldType == GUI_BUTTON_HOLD)
        && (_heldID != id)) {
      // clear hold if cursor moves off BUTTON_PRESS/BUTTON_HOLD
      _heldID = 0;
      _heldType = GUI_NULL;
      _needRender = true;
    }

    if (!buttonDown && (_heldID != 0)) {
      if ((type == GUI_BUTTON || type == GUI_CHECKBOX)
          && buttonEvent && (_heldID == id)) {
        // activate if cursor is over element & button is released
        _eventID = id;
        if (type == GUI_CHECKBOX) { ePtr->checkbox_set = !ePtr->checkbox_set; }
        usedEvent = true;
      }

      _heldID = 0;
      _heldType = GUI_NULL;
      _needRender = true;
    }
  }

  if (!isMenu(type) && _popupID != 0 && anyGuiButtonEvent) {
    // press/release off menu closes open menus
    deactivatePopups();
  }

  if (usedEvent) {
    win.removeEvent(EVENT_MOUSE_BUTTON1);
  }
}

void Gui::processCharEvent(Window& win)
{
  GuiElem* e = findElem(_focusID);
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
      setFocusID(win, next ? next->id : 0);
      usedEvent = true;
    } else if (c.key == KEY_TAB && c.mods == MOD_SHIFT) {
      const GuiElem* prev = findPrevElem(_focusID, GUI_ENTRY);
      setFocusID(win, prev ? prev->id : 0);
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

void Gui::setFocusID(Window& win, EventID id)
{
  if (_focusID == id) { return; }
  if (_textChanged) {
    _textChanged = false;
    _eventID = _focusID;
  }
  _focusID = id;
  if (id != 0) {
    _lastCursorUpdate = win.lastPollTime();
    const Panel* p = findPanel(_focusID);
    _cursorBlinkTime = p ? p->theme->cursorBlinkTime : 400000;
    _cursorState = true;
  }
  _needRender = true;
}

void Gui::setElemState(EventID id, bool enable)
{
  GuiElem* e = findElem(id);
  if (e && e->_enabled != enable) {
    e->_enabled = enable;
    _needRender = true;
  }
}

bool Gui::setText(EventID id, std::string_view text)
{
  for (auto& pPtr : _panels) {
    GuiElem* e = findElemT(&pPtr->root, id);
    if (!e) { continue; }

    e->text = text;
    pPtr->needLayout |= (e->type != GUI_ENTRY);
    _needRender = true;
    return true;
  }
  return false;
}

bool Gui::setBool(EventID id, bool val)
{
  for (auto& pPtr : _panels) {
    GuiElem* e = findElemT(&pPtr->root, id);
    if (!e) { continue; }

    if (e->type != GUI_CHECKBOX) { break; }
    e->checkbox_set = val;
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
  calcPos(thm, p.root, x, y, x + p.layout.w, y + p.layout.h);
  _needRender = true;
}

void Gui::initElem(GuiElem& def)
{
  if (isMenu(def.type)) { def.id = --_lastUniqueID; }
  for (GuiElem& e : def.elems) { initElem(e); }
}

bool Gui::drawElem(
  GuiElem& def, DrawContext& dc, DrawContext& dc2, int64_t usec,
  const GuiTheme& thm, const GuiTheme::Style* style) const
{
  bool needRedraw = false; // use for anim trigger later
  switch (def.type) {
    case GUI_BACKGROUND:
      assert(style != nullptr);
      drawRec(dc, def, style);
      break;
    case GUI_LABEL:
      assert(style != nullptr);
      dc2.color(style->textColor);
      dc2.text(TextFormatting{thm.font, float(thm.textSpacing)},
               def._x, def._y, ALIGN_TOP_LEFT, def.text);
      break;
    case GUI_HLINE: {
      const float b = thm.border;
      assert(style != nullptr);
      dc.color(style->textColor);
      dc.rectangle(def._x, def._y + b, def._w, def._h - (b*2));
      break;
    }
    case GUI_VLINE: {
      const float b = thm.border;
      assert(style != nullptr);
      dc.color(style->textColor);
      dc.rectangle(def._x + b, def._y, def._w - (b*2), def._h);
      break;
    }
    case GUI_BUTTON:
    case GUI_BUTTON_PRESS:
    case GUI_BUTTON_HOLD:
      if (!def._enabled) {
        style = &thm.buttonDisable;
      } else if (def.id == _heldID) {
        style = (def.id == _hoverID || def.type != GUI_BUTTON)
          ? &thm.buttonPress : &thm.buttonHold;
      } else {
        style = (def.id == _hoverID) ? &thm.buttonHover : &thm.button;
      }
      drawRec(dc, def, style);
      break;
    case GUI_CHECKBOX: {
      if (!def._enabled) {
        style = &thm.checkboxDisable;
      } else if (def.id == _heldID) {
        style = (def.id == _hoverID) ? &thm.checkboxPress : &thm.checkboxHold;
      } else {
        style = (def.id == _hoverID) ? &thm.checkboxHover : &thm.checkbox;
      }
      const float b = thm.border;
      const float cw = thm.font->calcWidth(thm.checkCode) + (b*2.0f);
      const float ch = float(thm.font->size() - 1) + (b*2.0f);
      drawRec(dc, def._x, def._y, cw, ch, style);
      if (def.checkbox_set) {
        dc2.color(style->textColor);
        dc2.glyph(TextFormatting{thm.font, float(thm.textSpacing)},
                  def._x + b + thm.checkXOffset, def._y + b + thm.checkYOffset,
                  ALIGN_TOP_LEFT, thm.checkCode);
      }
      break;
    }
    case GUI_MENU:
      style = def._active ? &thm.menuButtonOpen
        : ((def.id == _hoverID) ? &thm.menuButtonHover : &thm.menuButton);
      drawRec(dc, def, style);
      needRedraw |= drawElem(def.elems[0], dc, dc2, usec, thm, style);
      break;
    case GUI_MENU_ITEM:
      if (!def._enabled) {
        style = &thm.menuItemDisable;
      } else if (def.id == _hoverID) {
        style = &thm.menuItemSelect;
        drawRec(dc, def, style);
      }
      break;
    case GUI_SUBMENU: {
      if (def._active) {
        style = &thm.menuItemSelect;
        drawRec(dc, def, style);
      }
      needRedraw |= drawElem(def.elems[0], dc, dc2, usec, thm, style);
      const float b = thm.border;
      dc2.glyph(TextFormatting{thm.font, 0}, def._x + def._w, def._y + b,
                ALIGN_TOP_RIGHT, thm.subMenuCode);
      break;
    }
    case GUI_ENTRY: {
      if (!def._enabled) {
        style = &thm.entryDisable;
      } else {
        style = (def.id == _focusID) ? &thm.entryFocus : &thm.entry;
      }
      drawRec(dc, def, style);
      const std::string txt = (def.entry.type == ENTRY_PASSWORD)
        ? passwordStr(thm.passwordCode, def.text.size()) : def.text;
      const RGBA8 textColor = style->textColor;
      const float cw = thm.cursorWidth;
      const float tw = thm.font->calcWidth(txt);
      const float fs = float(thm.font->size());
      const float maxWidth = def._w - thm.entryLeftMargin
        - thm.entryRightMargin - cw;
      float tx = def._x + thm.entryLeftMargin;
      if (tw > maxWidth) {
        // text doesn't fit in entry
        const RGBA8 c0 = textColor & 0x00ffffff;
        if (def.id == _focusID) {
          // text being edited so show text end where cursor is
          dc2.hgradiant(def._x, c0, tx + (fs * .5f), textColor);
          tx -= tw - maxWidth;
        } else {
          // show text start when not in focus
          dc2.hgradiant(def._x + def._w - thm.entryRightMargin - (fs * .5f),
                        textColor, def._x + def._w, c0);
        }
      } else {
        dc2.color(textColor);
        if (HAlign(def.entry.align) == ALIGN_RIGHT) {
          tx = def._x + def._w - (tw + cw + thm.entryRightMargin);
        } else if (HAlign(def.entry.align) != ALIGN_LEFT) { // HCENTER
          tx = def._x + ((def._w - tw) * .5f);
        }
      }
      dc2.text(TextFormatting{thm.font, float(thm.textSpacing)},
               tx, def._y + thm.entryTopMargin, ALIGN_TOP_LEFT, txt,
               {def._x, def._y, def._w, def._h});
      if (def.id == _focusID && _cursorState) {
        // draw cursor
        dc.color(thm.cursorColor);
        dc.rectangle(tx + tw, def._y + thm.entryTopMargin, cw, fs - 1.0f);
      }
      break;
    }
    case GUI_IMAGE:
      dc.texture(def.image.texId);
      dc.rectangle(def._x + thm.border, def._y + thm.border,
                   def.image.width, def.image.height,
                   def.image.texCoord0, def.image.texCoord1);
      break;
    case GUI_HFRAME:
    case GUI_VFRAME:
      break; // layout only - nothing to draw
    default:
      GX_LOG_ERROR("unknown type ", def.type);
      break;
  }

  // draw child elements
  if (!isMenu(def.type)) {
    for (auto& e : def.elems) {
      needRedraw |= drawElem(e, dc, dc2, usec, thm, style);
    }
  }
  return needRedraw;
}

bool Gui::drawPopup(
  GuiElem& def, DrawContext& dc, DrawContext& dc2, int64_t usec,
  const GuiTheme& thm) const
{
  bool needRedraw = false; // use for anim trigger later
  if (isMenu(def.type)) {
    // menu frame & items
    if (def._active) {
      GuiElem& e1 = def.elems[1];
      needRedraw |= drawElem(e1, dc, dc2, usec, thm, &thm.menuFrame);
      // continue popup draw for possible active sub-menu
      needRedraw |= drawPopup(e1, dc, dc2, usec, thm);
    }
  } else {
    for (auto& e : def.elems) { needRedraw |= drawPopup(e, dc, dc2, usec, thm); }
  }
  return needRedraw;
}

Gui::Panel* Gui::findPanel(PanelID id)
{
  for (auto& pPtr : _panels) {
    if (findElemT(&pPtr->root, id)) { return pPtr.get(); }
  }
  return nullptr;
}

const Gui::Panel* Gui::findPanel(PanelID id) const
{
  for (auto& pPtr : _panels) {
    if (findElemT(&pPtr->root, id)) { return pPtr.get(); }
  }
  return nullptr;
}

GuiElem* Gui::findElem(EventID id)
{
  for (auto& pPtr : _panels) {
    GuiElem* e = findElemT(&pPtr->root, id);
    if (e) { return e; }
  }
  return nullptr;
}

const GuiElem* Gui::findElem(EventID id) const
{
  for (auto& pPtr : _panels) {
    const GuiElem* e = findElemT(&pPtr->root, id);
    if (e) { return e; }
  }
  return nullptr;
}

GuiElem* Gui::findNextElem(EventID id, GuiElemType type)
{
  assert(id != 0);
  Panel* p = findPanel(id);
  if (!p) { return nullptr; }

  std::vector<GuiElem*> stack;
  GuiElem* e = &p->root;
  GuiElem* next = nullptr;
  GuiElem* first = nullptr;
  for (;;) {
    if (e->id > 0 && e->_enabled && (type == GUI_NULL || e->type == type)) {
      if (e->id == (id+1)) { return e; }
      if (e->id > id && (!next || e->id < next->id)) { next = e; }
      if (!first || e->id < first->id) { first = e; }
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

GuiElem* Gui::findPrevElem(EventID id, GuiElemType type)
{
  assert(id != 0);
  Panel* p = findPanel(id);
  if (!p) { return nullptr; }

  std::vector<GuiElem*> stack;
  GuiElem* e = &p->root;
  GuiElem* prev = nullptr;
  GuiElem* last = nullptr;
  for (;;) {
    if (e->id > 0 && e->_enabled && (type == GUI_NULL || e->type == type)) {
      if (e->id == (id-1)) { return e; }
      if (e->id < id && (!prev || e->id > prev->id)) { prev = e; }
      if (!last || e->id > last->id) { last = e; }
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
