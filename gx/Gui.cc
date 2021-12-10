//
// gx/Gui.cc
// Copyright (C) 2021 Richard Bradley
//

// TODO: handle tab/enter/mouse select differently for entry
// TODO: cursor movement for entry
// TODO: allow right button to open menus & select menu items
// TODO: disable/enable menu items
//   - need theme 'disabled menu item text color'
//   - mouse over disabled items has no 'hover' change

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

static void deactivate(GuiElem& def)
{
  def._active = false;
  for (auto& e : def.elems) { deactivate(e); }
}

[[nodiscard]] static GuiElem* findElemByXY(
  GuiElem& def, float x, float y, bool popup)
{
  if (popup && def.type == GUI_MENU) {
    if (def._active) {
      GuiElem* e = findElemByXY(def.elems[1], x, y, false);
      if (e) { return e; }
    }
    if (contains(def, x, y)) { return &def; }
  }
  else if (popup || contains(def, x, y))
  {
    if (!popup && def.id != 0) { return &def; }
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

  if (style->edgeColor != 0) {
    dc.color(style->edgeColor);
    dc.rectangle(x, y, w, 1);
    dc.rectangle(x, y + h - 1, w, 1);
    dc.rectangle(x, y, 1, h);
    dc.rectangle(x + w - 1, y, 1, h);
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
      int lines = calcLines(def.text);
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
    default: {
      // align single child element
      const float b = thm.border;
      if (!def.elems.empty()) {
        assert(def.elems.size() == 1);
        calcPos(thm, def.elems[0], left + b, top + b, right - b, bottom - b);
      }
      break;
    }
  }
}

static std::string passwordStr(int32_t code, std::size_t len)
{
  std::string ch = toUTF8(code);
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
  _needRender = true;
  _popupActive = false;
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
  if (win.focused()
      && (win.allEvents() & (EVENT_MOUSE_MOVE | EVENT_MOUSE_BUTTON1))) {
    processMouseEvent(win);
  }

  if (_heldType == GUI_BUTTON_HOLD && _eventID == 0) {
    _eventID = _heldID;
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

    for (auto it = _panels.rbegin(), end = _panels.rend(); it != end; ++it) {
      Panel& p = **it;
      const TextFormatting tf{p.theme->font, float(p.theme->textSpacing)};
      drawElem(dc, dc2, tf, p.root, p, &(p.theme->panel));

      if (!dc2.empty()) {
        dc.append(dc2);
        dc2.clear();
      }
    }

    if (_popupActive) {
      for (auto it = _panels.rbegin(), end = _panels.rend(); it != end; ++it) {
        Panel& p = **it;
        const TextFormatting tf{p.theme->font, float(p.theme->textSpacing)};
        drawPopup(dc, dc2, tf, p.root, p);

        if (!dc2.empty()) {
          dc.append(dc2);
          dc2.clear();
        }
      }
    }
    _needRender = false;
    _needRedraw = true;
  }
}

void Gui::deactivatePopup()
{
  for (auto& pPtr : _panels) { deactivate(pPtr->root); }
  _popupActive = false;
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
      ePtr = findElemByXY(pPtr->root, win.mouseX(), win.mouseY(), _popupActive);
      if (ePtr) {
        id = ePtr->id;
        type = ePtr->type;
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
    EventID hid = (buttonDown && type != GUI_MENU_ITEM && id != _heldID)
      ? 0 : id;
    if (_hoverID != hid) {
      _hoverID = hid;
      _needRender = true;
    }
  }

  bool usedEvent = false;
  if (type == GUI_MENU) {
    if (pressEvent && ePtr->_active) {
      // click on open menu button closes it
      deactivatePopup();
      _needRender = true;
      usedEvent = true;
    } else if (pressEvent || _popupActive) {
      // open menu
      if (_popupActive) { deactivatePopup(); } // close other open menu
      ePtr->_active = true;
      _popupActive = true;
      _needRender = true;
      usedEvent = true;
    }
  } else if (type == GUI_MENU_ITEM) {
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

  if (type != GUI_MENU && _popupActive && anyGuiButtonEvent) {
    // press/release off menu closes open menus
    deactivatePopup();
    _needRender = true;
  }

  // clear button event if used by GUI
  if (buttonEvent && usedEvent) {
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
      std::string cb = getClipboardFirstLine();
      bool added = false;
      for (UTF8Iterator itr{cb}; !itr.done(); itr.next()) {
        added |= addEntryChar(*e, itr.get());
      }
      _needRender |= added;
      _textChanged |= added;
    } else if ((c.key == KEY_TAB && c.mods == 0) || c.key == KEY_ENTER) {
      GuiElem* next = findNextElem(_focusID, GUI_ENTRY);
      setFocusID(win, next ? next->id : 0);
      usedEvent = true;
    } else if (c.key == KEY_TAB && c.mods == MOD_SHIFT) {
      GuiElem* prev = findPrevElem(_focusID, GUI_ENTRY);
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

void Gui::layout(Panel& p, float x, float y, AlignEnum align)
{
  assert(p.theme->font != nullptr);
  p.layout = {x,y,0,0};
  if (HAlign(align) == ALIGN_RIGHT) { std::swap(p.layout.x, p.layout.w); }
  if (VAlign(align) == ALIGN_BOTTOM) { std::swap(p.layout.y, p.layout.h); }
  p.root.align = align;
  p.needLayout = true;
  _needRender = true;
}

void Gui::init(GuiElem& def)
{
  if (def.type == GUI_MENU) { def.id = --_lastUniqueID; }
  for (GuiElem& e : def.elems) { init(e); }
}

void Gui::drawElem(
  DrawContext& dc, DrawContext& dc2, const TextFormatting& tf,
  const GuiElem& def, const Panel& panel, const GuiTheme::Style* style) const
{
  const GuiTheme& thm = *panel.theme;
  switch (def.type) {
    case GUI_BACKGROUND:
      assert(style != nullptr);
      drawRec(dc, def, style);
      break;
    case GUI_LABEL:
      assert(style != nullptr);
      dc2.color(style->textColor);
      dc2.text(tf, def._x, def._y, ALIGN_TOP_LEFT, def.text);
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
      if (def.id == _heldID) {
        style = (def.id == _hoverID || def.type != GUI_BUTTON)
          ? &thm.buttonPress : &thm.buttonHold;
      } else {
        style = (def.id == _hoverID) ? &thm.buttonHover : &thm.button;
      }
      drawRec(dc, def, style);
      break;
    case GUI_CHECKBOX: {
      if (def.id == _heldID) {
        style = (def.id == _hoverID) ? &thm.checkboxPress : &thm.checkboxHold;
      } else {
        style = (def.id == _hoverID) ? &thm.checkboxHover : &thm.checkbox;
      }
      const float b = thm.border;
      const float cw = tf.font->calcWidth(thm.checkCode) + (b*2.0f);
      const float ch = float(tf.font->size() - 1) + (b*2.0f);
      drawRec(dc, def._x, def._y, cw, ch, style);
      if (def.checkbox_set) {
        dc2.color(style->textColor);
        dc2.glyph(tf, def._x + b + thm.checkXOffset,
                  def._y + b + thm.checkYOffset, ALIGN_TOP_LEFT, thm.checkCode);
      }
      break;
    }
    case GUI_MENU:
      style = def._active ? &thm.menuButtonOpen
        : ((def.id == _hoverID) ? &thm.menuButtonHover : &thm.menuButton);
      drawRec(dc, def, style);
      break;
    case GUI_MENU_ITEM:
      if (def.id == _hoverID) {
        style = &thm.menuItemSelect;
        drawRec(dc, def, style);
      }
      break;
    case GUI_ENTRY: {
      style = (def.id == _focusID) ? &thm.entryFocus : &thm.entry;
      drawRec(dc, def, style);
      const std::string txt = (def.entry.type == ENTRY_PASSWORD)
        ? passwordStr(thm.passwordCode, def.text.size()) : def.text;
      const RGBA8 textColor = style->textColor;
      const float cw = thm.cursorWidth;
      const float tw = tf.font->calcWidth(txt);
      const float fs = float(tf.font->size());
      const float maxWidth = def._w - thm.entryLeftMargin
        - thm.entryRightMargin - cw;
      float tx = def._x + thm.entryLeftMargin;
      if (tw > maxWidth) {
        // text doesn't fit in entry
        tx -= tw - maxWidth;
        dc2.hgradiant(def._x + 1.0f, textColor & 0x00ffffff,
                      def._x + (fs*.5f), textColor);
        // TODO: gradiant dim at both ends if moving cursor in long string
      } else {
        dc2.color(textColor);
        if (HAlign(def.entry.align) == ALIGN_RIGHT) {
          tx = def._x + def._w - (tw + cw + thm.entryRightMargin);
        } else if (HAlign(def.entry.align) != ALIGN_LEFT) { // HCENTER
          tx = def._x + ((def._w - tw) * .5f);
        }
      }
      dc2.text(tf, tx, def._y + thm.entryTopMargin, ALIGN_TOP_LEFT, txt,
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
  if (def.type == GUI_MENU) {
    // menu button label
    drawElem(dc, dc2, tf, def.elems[0], panel, style);
  } else {
    for (auto& e : def.elems) { drawElem(dc, dc2, tf, e, panel, style); }
  }
}

void Gui::drawPopup(
  DrawContext& dc, DrawContext& dc2, const TextFormatting& tf,
  const GuiElem& def, const Panel& panel) const
{
  if (def.type == GUI_MENU) {
    if (def._active) {
      // menu frame & items
      drawElem(dc, dc2, tf, def.elems[1], panel, &panel.theme->menuFrame);
    }
  } else {
    for (auto& e : def.elems) { drawPopup(dc, dc2, tf, e, panel); }
  }
}

Gui::Panel* Gui::findPanel(EventID id)
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
    if (e->id > 0 && (type == GUI_NULL || e->type == type)) {
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
    if (e->id > 0 && (type == GUI_NULL || e->type == type)) {
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
