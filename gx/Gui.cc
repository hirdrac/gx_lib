//
// gx/Gui.cc
// Copyright (C) 2020 Richard Bradley
//

#include "Gui.hh"
#include "Window.hh"
#include "Font.hh"
#include "print.hh"
#include "Logger.hh"
#include <algorithm>
#include <cassert>

namespace {
  constexpr bool contains(const gx::GuiElem& e, float x, float y)
  {
    return (x >= e._x) && (x < (e._x + e._w))
      && (y >= e._y) && (y < (e._y + e._h));
  }
}


gx::Gui::Gui(const GuiElem& rootElem) : _rootElem(rootElem) { }
gx::Gui::Gui(GuiElem&& rootElem) : _rootElem(std::move(rootElem)) { }

void gx::Gui::layout(float x, float y, AlignEnum align, int border)
{
  if (_x != x || _y != y || _align != align) {
    _x = x;
    _y = y;
    _align = align;
    _needPos = true;
    _needRender = true;
  }

  if (border != _border) {
    _border = border;
    _needSize = true;
    _needPos = true;
    _needRender = true;
  }
}

void gx::Gui::update(Window& win)
{
  if (_needSize) {
    calcSize(_rootElem);
    _needSize = false;
  }

  if (_needPos) {
    // FIXME - adjust _x/_y based on alignment?
    calcPos(_rootElem, _x, _y);
    _needPos = false;
  }

  // button event & state update
  bool held = win.buttons() & BUTTON1;
  GuiElem* ptr = findEventElem(win.mouseX(), win.mouseY());
  int id = ptr ? ptr->eventID : 0;
  if (_buttonHoverID != id) {
    _buttonHoverID = id;
    _needRender = true;
  }

  if (held && (win.events() & EVENT_MOUSE_BUTTON)) {
    _buttonPressedID = id;
    _lastPressedID = id;
    _needRender = true;
  } else {
    _buttonPressedID = 0;
  }

  if (!held && _lastPressedID == id) {
    _buttonReleasedID = id;
    _lastPressedID = 0;
  } else {
    if (_buttonReleasedID) { _needRender = true; }
    _buttonReleasedID = 0;
  }

  if (held) {
    _buttonHeldID = _lastPressedID;
  } else {
    _buttonHeldID = 0;
    if (_lastPressedID) { _needRender = true; }
    _lastPressedID = 0;
  }

  if (contains(_rootElem, win.mouseX(), win.mouseY())) {
    win.consumeEvent(gx::EVENT_MOUSE_BUTTON);
  }

  // redraw GUI if needed
  if (_needRender) {
    _dl.clear();
    drawGfx(GFX_FLAT_BG, _rootElem);
    drawElem(_rootElem, BSTATE_NONE);
    _needRender = false;
    _needRedraw = true;
  } else {
    _needRedraw = false;
  }
}

void gx::Gui::calcSize(GuiElem& def)
{
  switch (def.type) {
    case GUI_HFRAME: {
      float total_w = 0, max_h = 0;
      for (GuiElem& e : def.elems) {
        calcSize(e);
        total_w += e._w + _border;
        max_h = std::max(max_h, e._h);
      }
      def._w = total_w + _border;
      def._h = max_h + (_border * 2);
      break;
    }
    case GUI_VFRAME: {
      float total_h = 0, max_w = 0;
      for (GuiElem& e : def.elems) {
        calcSize(e);
        total_h += e._h + _border;
        max_w = std::max(max_w, e._w);
      }
      def._w = max_w + (_border * 2);
      def._h = total_h + _border;
      break;
    }
    case GUI_LABEL: {
      assert(def.pFont != nullptr);
      const Font& fnt = *def.pFont;
      def._w = fnt.calcWidth(def.text);
      int lines = fnt.calcLines(def.text);
      def._h = (fnt.size() - 1) * lines + (_spacing * std::max(lines - 1, 0));
      // FIXME - improve line height calc (based on font ymax/ymin?)
      break;
    }
    case GUI_HLINE:
      def._w = 32 + _border*2;
      def._h = 1 + _border*2;
      break;
    case GUI_VLINE:
      def._w = 1 + _border*2;
      def._h = 32 + _border*2;
      break;
    case GUI_BUTTON: {
      GuiElem& e = def.elems[0];
      calcSize(e);
      def._w = e._w + (_border * 2);
      def._h = e._h + (_border * 2);
      break;
    }
    case GUI_TOGGLE:
      // FIXME - finish
      break;
    default:
      LOG_ERROR("unknown type");
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
        base_x += _border;
        float yy = 0;
        if ((e.align & ALIGN_VCENTER) == ALIGN_VCENTER) {
          yy = (def._h - e._h) / 2.0f;
        } else if (e.align & ALIGN_BOTTOM) {
          yy = (def._h - e._h) - _border;
        } else {
          yy = _border;
        }
        calcPos(e, base_x, base_y + yy);
        base_x += e._w;
      }
      break;
    case GUI_VFRAME:
      for (GuiElem& e : def.elems) {
        base_y += _border;
        float xx = 0;
        if ((e.align & ALIGN_HCENTER) == ALIGN_HCENTER) {
          xx = (def._w - e._w) / 2.0f;
        } else if (e.align & ALIGN_RIGHT) {
          xx = (def._w - e._w) - _border;
        } else {
          xx = _border;
        }
        calcPos(e, base_x + xx, base_y);
        base_y += e._h;
      }
      break;
    case GUI_BUTTON:
      calcPos(def.elems[0], base_x + _border, base_y + _border);
      break;
    case GUI_TOGGLE:
      // FIXME - finish
      break;
    default:
      break;
  }
}

void gx::Gui::drawElem(GuiElem& def, ButtonState bstate)
{
  // FIXME - add style option to HFRAME,VFRAME,BUTTON

  switch (def.type) {
    case GUI_HFRAME:
      //_dl.color(.1,.1,.2);
      //_dl.rectangle(def._x, def._y, def._w, def._h);
      break;
    case GUI_VFRAME:
      //_dl.color(.1,.2,.1);
      //_dl.rectangle(def._x, def._y, def._w, def._h);
      break;
    case GUI_LABEL:
      _dl.color(_colorText);
      _dl.text(
        *def.pFont, def._x, def._y, gx::ALIGN_TOP_LEFT, _spacing, def.text);
      break;
    case GUI_HLINE:
    case GUI_VLINE:
      _dl.color(_colorText);
      _dl.rectangle(def._x + _border, def._y + _border, def._w - (_border*2), def._h - (_border*2));
      break;
    case GUI_BUTTON:
      if (def.eventID == _buttonHeldID) {
        if (def.eventID == _buttonHoverID) {
          _dl.color(_colorButtonPressed);
          bstate = BSTATE_PRESSED;
        } else {
          _dl.color(_colorButtonHeldOnly);
          bstate = BSTATE_HELD_ONLY;
        }
      } else if (def.eventID == _buttonHoverID && !_buttonHeldID) {
        _dl.color(_colorButtonHover);
        bstate = BSTATE_HOVER;
      } else {
        _dl.color(_colorButtonNormal);
        bstate = BSTATE_NORMAL;
      }
      _dl.rectangle(def._x, def._y, def._w, def._h);
      break;
    case GUI_TOGGLE:
      // FIXME - finish
      break;
    default:
      LOG_ERROR("unknown type");
      break;
  }

  for (GuiElem& e : def.elems) {
    drawElem(e, bstate);
  }
}

gx::GuiElem* gx::Gui::findEventElem(float x, float y)
{
  gx::GuiElem* ptr = &_rootElem;
  gx::GuiElem* end = ptr + 1;
  while (ptr != end) {
    GuiElem& e = *ptr;
    if (!contains(e, x, y)) {
      ++ptr;
      continue;
    }

    if (e.eventID > 0) {
      return ptr;
    }

    ptr = e.elems.data();
    end = ptr + e.elems.size();
  }

  return nullptr;
}

gx::GuiElem* gx::Gui::findElem(int eventID)
{
  std::vector stack = { &_rootElem };
  while (!stack.empty()) {
    auto e = stack.back();
    if (e->eventID == eventID) {
      return e;
    }

    stack.pop_back();
    for (auto& x : e->elems) {
      stack.push_back(&x);
    }
  }

  return nullptr;
}

void gx::Gui::drawGfx(GfxEnum gfx, float x, float y, float w, float h)
{
  switch (gfx) {
    case GFX_FLAT_BG:
      _dl.color(_colorBackground);
      _dl.rectangle(x, y, w, h);
      break;
    case GFX_RAISED_BG:
      _dl.color(_colorBackground);
      _dl.rectangle(x, y, w, h);
      _dl.color(.8,.8,.8);
      _dl.line(x+.5, y+.5, x+w-.5, y+.5);
      _dl.line(x+.5, y+.5, x+.5, y+h-.5);
      _dl.color(0,0,0);
      _dl.line(x+w-.5, y+h-.5, x+.5, y+h-.5);
      _dl.line(x+w-.5, y+h-.5, x+w-.5, y+.5);
      break;
    default:
      break;
  }
}
