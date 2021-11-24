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

namespace {
  [[nodiscard]] bool contains(const gx::GuiElem& e, float x, float y)
  {
    return (x >= e._x) && (x < (e._x + e._w))
      && (y >= e._y) && (y < (e._y + e._h));
  }

  [[nodiscard]] float actualX(float x, float w, gx::AlignEnum a)
  {
    if (gx::HAlign(a) == gx::ALIGN_LEFT) {
      return x;
    } else if (gx::HAlign(a) == gx::ALIGN_RIGHT) {
      return x - w;
    } else {
      return x - (w / 2.0f);
    }
  }

  [[nodiscard]] float actualY(float y, float h, gx::AlignEnum a)
  {
    if (gx::VAlign(a) == gx::ALIGN_TOP) {
      return y;
    } else if (gx::VAlign(a) == gx::ALIGN_BOTTOM) {
      return y - h;
    } else {
      return y - (h / 2.0f);
    }
  }

  [[nodiscard]] int calcLines(std::string_view text)
  {
    if (text.empty()) { return 0; }
    int lines = 1;
    for (int ch : text) { lines += (ch == '\n'); }
    return lines;
  }

  void deactivate(gx::GuiElem& def)
  {
    def._active = false;
    for (auto& e : def.elems) { deactivate(e); }
  }

  [[nodiscard]] gx::GuiElem* findElemByXY(
    gx::GuiElem& def, float x, float y, bool popup)
  {
    if (popup && def.type == gx::GUI_MENU) {
      if (def._active) {
        gx::GuiElem* e = findElemByXY(def.elems[1], x, y, false);
        if (e) { return e; }
      }
      if (contains(def, x, y)) { return &def; }
    }
    else if (popup || contains(def, x, y))
    {
      if (!popup && def.id != 0) { return &def; }
      for (gx::GuiElem& c : def.elems) {
        gx::GuiElem* e = findElemByXY(c, x, y, popup);
        if (e) { return e; }
      }
    }
    return nullptr;
  }

  bool addEntryChar(gx::GuiElem& e, int32_t codepoint)
  {
    assert(e.type == gx::GUI_ENTRY);

    if (e.entry.maxLength != 0
        && gx::lengthUTF8(e.text) >= e.entry.maxLength) {
      return false; // no space for character
    }

    switch (e.entry.type) {
      default: // ENTRY_TEXT, ENTRY_PASSWORD
        if (codepoint <= 31) { return false; }
        break;
      case gx::ENTRY_CARDINAL:
        if (!std::isdigit(codepoint)
            || (e.text == "0" && codepoint == '0')) { return false; }
        if (e.text == "0") { e.text.clear(); }
        break;
      case gx::ENTRY_INTEGER:
        if ((!std::isdigit(codepoint) && codepoint != '-')
            || (codepoint == '-' && !e.text.empty() && e.text != "0")
            || (codepoint == '0' && (e.text == "0" || e.text == "-"))) {
          return false; }
        if (e.text == "0") { e.text.clear(); }
        break;
      case gx::ENTRY_FLOAT:
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

    e.text += gx::toUTF8(codepoint);
    return true;
  }

  void drawRec(gx::DrawContext& dc, const gx::GuiElem& def,
               const gx::GuiTheme::Style& style)
  {
    if (style.backgroundColor != 0) {
      dc.color(style.backgroundColor);
      dc.rectangle(def._x, def._y, def._w, def._h);
    }

    if (style.edgeColor != 0) {
      dc.color(style.edgeColor);
      dc.rectangle(def._x, def._y, def._w, 1);
      dc.rectangle(def._x, def._y + def._h - 1, def._w, 1);
      dc.rectangle(def._x, def._y, 1, def._h);
      dc.rectangle(def._x + def._w - 1, def._y, 1, def._h);
    }
  }
}


gx::Gui::Gui(const GuiElem& rootElem)
  : _rootElem(rootElem) { init(_rootElem); }

gx::Gui::Gui(GuiElem&& rootElem)
  : _rootElem(std::move(rootElem)) { init(_rootElem); }

void gx::Gui::layout(const GuiTheme& theme, float x, float y, AlignEnum align)
{
  _theme = &theme;
  assert(_theme->font != nullptr);

  _pt.set(x, y);
  _rootElem.align = align;
  _needLayout = true;
  _needRender = true;
}

void gx::Gui::update(Window& win)
{
  // clear event state that only persists for a single update
  _eventID = 0;
  _needRedraw = false;

  // size & position update
  if (_needLayout) {
    calcSize(_rootElem);
    const float x = actualX(_pt.x, _rootElem._w, _rootElem.align);
    const float y = actualY(_pt.y, _rootElem._h, _rootElem.align);
    calcPos(_rootElem, x, y);
    _needLayout = false;
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

    const uint64_t bt = _theme->cursorBlinkTime;
    if (bt > 0) {
      // check for cursor blink
      const int64_t blinks = (win.lastPollTime() - _lastCursorUpdate) / bt;
      if (blinks > 0) {
        _lastCursorUpdate += blinks * bt;
        if (blinks & 1) { _cursorState = !_cursorState; }
        _needRender = true;
      }
    }
  }

  // redraw GUI if needed
  if (_needRender) {
    // layers:
    // 0 - background/frames
    // 1 - text
    // 2 - popup background/frames
    // 3 - popup text

    TextFormatting tf{_theme->font};
    tf.spacing = _theme->spacing;

    DrawContext dc0{_dlm[0]}, dc1{_dlm[1]};
    dc0.clear();
    dc1.clear();

    drawRec(dc0, _rootElem, _theme->base);
    drawElem(dc0, dc1, tf, _rootElem);

    if (_popupActive) {
      DrawContext dc2{_dlm[2]}, dc3{_dlm[3]};
      dc2.clear();
      dc3.clear();

      drawPopup(dc2, dc3, tf, _rootElem);
    } else {
      // clear popups
      auto itr = _dlm.find(2);
      if (itr != _dlm.end()) { itr->second.clear(); }
      itr = _dlm.find(3);
      if (itr != _dlm.end()) { itr->second.clear(); }
    }

    _needRender = false;
    _needRedraw = true;
  }
}

void gx::Gui::processMouseEvent(Window& win)
{
  const bool buttonDown = win.buttons() & BUTTON1;
  const bool buttonEvent = win.events() & EVENT_MOUSE_BUTTON1;
  const bool pressEvent = buttonDown & buttonEvent;
  const bool anyGuiButtonEvent = win.allEvents() & EVENT_MOUSE_BUTTON1;

  // get elem at mouse pointer
  GuiElem* ptr = nullptr;
  int id = 0;
  GuiElemType type = GUI_NULL;
  if (win.mouseIn()) {
    ptr = findElemByXY(_rootElem, win.mouseX(), win.mouseY(), _popupActive);
    if (ptr) {
      id = ptr->id;
      type = ptr->type;
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
    int hid = (buttonDown && type != GUI_MENU_ITEM && id != _heldID) ? 0 : id;
    if (_hoverID != hid) {
      _hoverID = hid;
      _needRender = true;
    }
  }

  bool usedEvent = false;
  if (type == GUI_MENU) {
    if (pressEvent && ptr->_active) {
      // click on open menu button closes it
      deactivate(_rootElem);
      _popupActive = false;
      _needRender = true;
      usedEvent = true;
    } else if (pressEvent || _popupActive) {
      // open menu
      if (_popupActive) { deactivate(_rootElem); } // close other open menu
      ptr->_active = true;
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
      if ((type == GUI_BUTTON) && buttonEvent && (_heldID == id)) {
        _eventID = id;
        usedEvent = true;
      }

      _heldID = 0;
      _heldType = GUI_NULL;
      _needRender = true;
    }
  }

  if (type != GUI_MENU && _popupActive && anyGuiButtonEvent) {
    // press/release off menu closes open menus
    deactivate(_rootElem);
    _popupActive = false;
    _needRender = true;
  }

  // clear button event if used by GUI
  if (buttonEvent && usedEvent) {
    win.removeEvent(EVENT_MOUSE_BUTTON1);
  }
}

void gx::Gui::processCharEvent(Window& win)
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
      std::string cb = getClipboard();
      std::string_view line{cb.data(), cb.find('\n')};
      bool added = false;
      for (UTF8Iterator itr{line}; !itr.done(); itr.next()) {
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

void gx::Gui::setFocusID(Window& win, int id)
{
  if (_focusID == id) { return; }
  if (_textChanged) {
    _textChanged = false;
    _eventID = _focusID;
  }
  _focusID = id;
  _lastCursorUpdate = win.lastPollTime();
  _cursorState = true;
  _needRender = true;
}

bool gx::Gui::setText(int id, std::string_view text)
{
  GuiElem* e = findElem(id);
  if (!e) { return false; }

  e->text = text;
  _needLayout = true;
  _needRender = true;
  return true;
}

void gx::Gui::init(GuiElem& def)
{
  if (def.type == GUI_MENU) {
    def.id = _uniqueID--;
  }

#ifndef NDEBUG
  switch (def.type) {
    case GUI_BUTTON:
    case GUI_BUTTON_PRESS:
    case GUI_BUTTON_HOLD:
    case GUI_MENU_ITEM:
      assert(def.elems.size() == 1);
      break;
    case GUI_MENU:
      assert(def.elems.size() == 2);
      break;
    default:
      break;
  }
#endif

  for (GuiElem& e : def.elems) { init(e); }
}

void gx::Gui::calcSize(GuiElem& def)
{
  switch (def.type) {
    case GUI_HFRAME: {
      float total_w = 0, max_h = 0;
      for (GuiElem& e : def.elems) {
        calcSize(e);
        total_w += e._w + _theme->border;
        max_h = std::max(max_h, e._h);
      }
      for (GuiElem& e : def.elems) {
        if (e.align & ALIGN_VJUSTIFY) { e._h = max_h; }
        // TODO: support horizontal justify
      }
      def._w = total_w + _theme->border;
      def._h = max_h + (_theme->border * 2);
      break;
    }
    case GUI_VFRAME: {
      float total_h = 0, max_w = 0;
      for (GuiElem& e : def.elems) {
        calcSize(e);
        total_h += e._h + _theme->border;
        max_w = std::max(max_w, e._w);
      }
      for (GuiElem& e : def.elems) {
        if (e.align & ALIGN_HJUSTIFY) { e._w = max_w; }
        // TODO: support vertical justify
      }
      def._w = max_w + (_theme->border * 2);
      def._h = total_h + _theme->border;
      break;
    }
    case GUI_LABEL: {
      const Font& fnt = *_theme->font;
      def._w = fnt.calcWidth(def.text);
      int lines = calcLines(def.text);
      def._h = float(
        (fnt.size() - 1) * lines + (_theme->spacing * std::max(lines - 1, 0)));
      // FIXME: improve line height calc (based on font ymax/ymin?)
      break;
    }
    case GUI_HLINE:
      def._w = float(32 + _theme->border * 2);
      def._h = float(1 + _theme->border * 2);
      break;
    case GUI_VLINE:
      def._w = float(1 + _theme->border * 2);
      def._h = float(32 + _theme->border * 2);
      break;
    case GUI_BUTTON:
    case GUI_BUTTON_PRESS:
    case GUI_BUTTON_HOLD:
    case GUI_MENU_ITEM: {
      GuiElem& e = def.elems[0];
      calcSize(e);
      def._w = float(e._w + (_theme->border * 2));
      def._h = float(e._h + (_theme->border * 2));
      break;
    }
    case GUI_MENU: {
      // menu button
      GuiElem& e = def.elems[0];
      calcSize(e);
      def._w = float(e._w + (_theme->border * 2));
      def._h = float(e._h + (_theme->border * 2));
      // menu items
      calcSize(def.elems[1]);
      break;
    }
    case GUI_ENTRY: {
      const Font& fnt = *_theme->font;
      if (def.entry.type == ENTRY_CARDINAL
          || def.entry.type == ENTRY_INTEGER
          || def.entry.type == ENTRY_FLOAT) {
        def._w = def.entry.size * fnt.digitWidth();
      } else {
        def._w = def.entry.size * fnt.calcWidth("A");
        // FIXME: use better width value than capital A * size
      }
      def._w += float(_theme->entryLeftMargin + _theme->entryRightMargin
                      + _theme->cursorWidth + 1);
      def._h = float(fnt.size() - 1)
        + _theme->entryTopMargin + _theme->entryBottomMargin;
      break;
    }
    case GUI_IMAGE:
      def._w = def.image.width + (_theme->border * 2);
      def._h = def.image.height + (_theme->border * 2);
      break;
    default:
      GX_LOG_ERROR("unknown type ", def.type);
      break;
  }
}

void gx::Gui::calcPos(GuiElem& def, float base_x, float base_y)
{
  def._x = base_x;
  def._y = base_y;

  switch (def.type) {
    case GUI_HFRAME:
      for (GuiElem& e : def.elems) {
        base_x += _theme->border;
        float yy = 0;
        if (VAlign(e.align) == ALIGN_TOP) {
          yy = _theme->border;
        } else if (VAlign(e.align) == ALIGN_BOTTOM) {
          yy = (def._h - e._h) - _theme->border;
        } else {
          yy = (def._h - e._h) / 2.0f;
        }
        // TODO: support horizontal alignment
        calcPos(e, base_x, base_y + yy);
        base_x += e._w;
      }
      break;
    case GUI_VFRAME:
      for (GuiElem& e : def.elems) {
        base_y += _theme->border;
        float xx = 0;
        if (HAlign(e.align) == ALIGN_LEFT) {
          xx = _theme->border;
        } else if (HAlign(e.align) == ALIGN_RIGHT) {
          xx = (def._w - e._w) - _theme->border;
        } else {
          xx = (def._w - e._w) / 2.0f;
        }
        // TODO: support vertical alignment
        calcPos(e, base_x + xx, base_y);
        base_y += e._h;
      }
      break;
    case GUI_BUTTON:
    case GUI_BUTTON_PRESS:
    case GUI_BUTTON_HOLD:
    case GUI_MENU_ITEM:
      // TODO: support alignment for child element
      calcPos(def.elems[0], base_x + _theme->border, base_y + _theme->border);
      break;
    case GUI_MENU:
      calcPos(def.elems[0], base_x + _theme->border, base_y + _theme->border);
      // FIXME: menu items always directly under button for now
      calcPos(def.elems[1], base_x, base_y + def._h);
      break;
    case GUI_LABEL:
    case GUI_HLINE:
    case GUI_VLINE:
    case GUI_ENTRY:
    case GUI_IMAGE:
      // noting extra to do
      break;
    default:
      GX_LOG_ERROR("unknown type ", def.type);
      break;
  }
}

void gx::Gui::drawElem(
  DrawContext& dc, DrawContext& dc2, const TextFormatting& tf,
  const GuiElem& def, const GuiTheme::Style* style) const
{
  if (!style) { style = &_theme->base; }
  switch (def.type) {
    case GUI_LABEL:
      dc2.color(style->textColor);
      dc2.text(tf, def._x, def._y, ALIGN_TOP_LEFT, def.text);
      break;
    case GUI_HLINE:
    case GUI_VLINE:
      dc.color(style->textColor);
      dc.rectangle(def._x + _theme->border, def._y + _theme->border,
                   def._w - (_theme->border*2), def._h - (_theme->border*2));
      break;
    case GUI_BUTTON:
    case GUI_BUTTON_PRESS:
    case GUI_BUTTON_HOLD:
      if (def.id == _heldID) {
        style = (def.id == _hoverID || def.type != GUI_BUTTON)
          ? &_theme->buttonPress : &_theme->buttonHold;
      } else {
        style = (def.id == _hoverID)
          ? &_theme->buttonHover : &_theme->button;
      }
      drawRec(dc, def, *style);
      break;
    case GUI_MENU:
      style = def._active ? &_theme->menuButtonOpen
        : ((def.id == _hoverID)
           ? &_theme->menuButtonHover : &_theme->menuButton);
      drawRec(dc, def, *style);
      break;
    case GUI_MENU_ITEM:
      if (def.id == _hoverID) {
        style = &_theme->menuItemSelect;
        drawRec(dc, def, *style);
      }
      break;
    case GUI_ENTRY: {
      const Font& fnt = *tf.font;
      float tw = fnt.calcWidth(def.text);
      if (def.id == _focusID) {
        style = &_theme->entryFocus;
        tw += float(1 + _theme->cursorWidth);
        // TODO: handle variable cursor position
      } else {
        style = &_theme->entry;
      }
      drawRec(dc, def, *style);
      const float maxWidth = def._w
        - _theme->entryLeftMargin - _theme->entryRightMargin;
      float tx = def._x + _theme->entryLeftMargin;
      const uint32_t textColor = style->textColor;
      if (tw > maxWidth) {
        // text doesn't fit in entry
        tx -= tw - maxWidth;
        dc2.hgradiant(def._x + 1.0f, textColor & 0x00ffffff,
                      def._x + float(fnt.size() / 2), textColor);
        // TODO: gradiant dim at both ends if moving cursor in long string
      } else {
        dc2.color(textColor);
      }
      dc2.text(tf, tx, def._y + _theme->entryTopMargin, ALIGN_TOP_LEFT,
               def.text, {def._x, def._y, def._w, def._h});
      // TODO: draw all characters as '*' for password entries
      if (def.id == _focusID && _cursorState) {
        // draw cursor
        dc.color(_theme->cursorColor);
        dc.rectangle(
          tx + tw - float(_theme->cursorWidth), def._y + _theme->entryTopMargin,
          float(_theme->cursorWidth), float(fnt.size()-1));
      }
      break;
    }
    case GUI_IMAGE:
      dc.texture(def.image.texId);
      dc.rectangle(def._x + _theme->border, def._y + _theme->border,
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
    drawElem(dc, dc2, tf, def.elems[0], style);
  } else {
    for (auto& e : def.elems) { drawElem(dc, dc2, tf, e, style); }
  }
}

void gx::Gui::drawPopup(
  DrawContext& dc, DrawContext& dc2, const TextFormatting& tf,
  const GuiElem& def) const
{
  if (def.type == GUI_MENU) {
    if (def._active) {
      // menu frame & items
      const GuiTheme::Style* style = &_theme->menuFrame;
      drawRec(dc, def.elems[1], *style);
      drawElem(dc, dc2, tf, def.elems[1], style);
    }
  } else {
    for (auto& e : def.elems) { drawPopup(dc, dc2, tf, e); }
  }
}

namespace {
  template<class T>
  inline T* findElemT(T* root, int id)
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
}

gx::GuiElem* gx::Gui::findElem(int id)
{
  return findElemT(&_rootElem, id);
}

const gx::GuiElem* gx::Gui::findElem(int id) const
{
  return findElemT(&_rootElem, id);
}

gx::GuiElem* gx::Gui::findNextElem(int id, GuiElemType type)
{
  assert(id != 0);
  std::vector<GuiElem*> stack;
  GuiElem* e = &_rootElem;
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

gx::GuiElem* gx::Gui::findPrevElem(int id, GuiElemType type)
{
  assert(id != 0);
  std::vector<GuiElem*> stack;
  GuiElem* e = &_rootElem;
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
