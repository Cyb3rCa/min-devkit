/// @file	
///	@ingroup 	minexamples
///	@copyright	Copyright (c) 2016, Cycling '74
/// @author		Timothy Place
///	@license	Usage of this file and its contents is governed by the MIT License

#include "c74_min.h"

using namespace c74::min;

class min_textslider : public object<min_textslider>, public ui_operator {
public:

	MIN_DESCRIPTION { "Display a text label" };
	MIN_TAGS		{ "ui" };
	MIN_AUTHOR		{ "Cycling '74" };
	MIN_RELATED		{ "comment, umenu, textbutton" };
	
	inlet<>	input	{ this, "(number) value to set" };
	outlet<> output	{ this, "(number) value" };


	min_textslider(const atoms& args = {})
	: ui_operator::ui_operator { this, args }
	{}


	message<> m_number { this, "number",
		MIN_FUNCTION {
			set(args);
			bang();
			redraw();
			return {};
		}
	};


	message<> set { this, "set",
		MIN_FUNCTION {
			m_unclipped_value = args[0];
			m_value = MIN_CLAMP(m_unclipped_value, m_range[0], m_range[1]);

			//			if ((x->attrShowValue) && (x->mouseOver))
			//				textslider_updatestringvalue(x);
			//			else
			//				jbox_redraw((t_jbox*)x);

			return {};
		}
	};


	message<> bang { this, "bang",
		MIN_FUNCTION {
			output.send(m_unclipped_value);
			return {};
		}
	};


	message<> m_notify { this, "notify",
		MIN_FUNCTION {
			symbol	s = args[1];
			symbol	msg = args[2];
			void*	sender = args[3];
			void*	data = args[4];

			if (sender == maxobj() && msg == "attr_modified") {
				update_text();
				redraw();
			}

			// TODO: the following should be baked-in for ui operators
			return { c74::max::jbox_notify((c74::max::t_jbox*)maxobj(), s, msg, sender, data) };
		}
	};


	attribute<number>	m_default	{ this, "defaultvalue", 0.0 };
	attribute<numbers>	m_range		{ this, "range", {{0.0, 1.0}},
		setter { MIN_FUNCTION {
			number low	{ args[0] };
			number high	{ args[1] };
			m_range_delta = high - low;
			return args;
		}}
	};
	attribute<numbers>	m_offset	{ this, "offset", {{10.0, 4.0}} };
	attribute<symbol>	m_label		{ this, "label", "" };
	attribute<symbol>	m_unit		{ this, "unit", "" };
	attribute<symbol>	m_fontname	{ this, "fontname", "lato-light" };
	attribute<number>	m_fontsize	{ this, "fontsize", 12.0 };
	attribute<bool>		m_showvalue	{ this, "showvalue", true };
	attribute<bool>		m_clickjump	{ this, "clickjump", true };

	enum class tracking {
		horizontal,
		vertical,
		both,
		enum_count
	};

	enum_map tracking_info = {
		"horizontal",
		"vertical",
		"both"
	};

	attribute<tracking> m_tracking { this, "tracking", tracking::horizontal, tracking_info,
		description { "Mouse tracking direction." }
	};


	message<> mouseenter { this, "mouseenter",
		MIN_FUNCTION {
			m_mouseover = true;
			if (m_showvalue)
				update_text();
			return {};
		}
	};

	message<> mouseleave { this, "mouseleave",
		MIN_FUNCTION {
			m_mouseover = false;
			if (m_showvalue)
				update_text();
			return {};
		}
	};

	message<> mousedown { this, "mousedown",
		MIN_FUNCTION {
			ui::target	t { args };
			number		x { args[2] };
			number		y { args[3] };
			int			modifiers { args[4] };

			// cache mouse position so we can restore it after we are done
			m_mouse_position[0] = t.x() + x;
			m_mouse_position[1] = t.y() + y;

			// Jump to new value on mouse down?
			if (m_clickjump) {
				auto delta = MIN_CLAMP((x - 1.0), 0.0, t.width() - 3.0);		// substract for borders
				delta = delta / (t.width() - 2.0) * m_range_delta + m_range[0];
				if (modifiers & c74::max::eCommandKey)
					m_number(static_cast<long>(delta));							//when command-key pressed, jump to the nearest integer-value
				else
					m_number(delta);											// otherwise jump to a float value
			}

			m_anchor = m_value;
//			c74::max::jbox_set_mousedragdelta(maxobj(), 1);
			return {};
		}
	};

	message<> mouseup { this, "mouseup",
		MIN_FUNCTION {
			ui::target	t { args };

			// restore mouse position
			c74::max::jmouse_setposition_view(args[1],
									t.x() + ((m_value - m_range[0]) / m_range_delta) * (t.width() - 2.0) + 1.0,
									m_mouse_position[1]);
			redraw();
			return {};
		}
	};

	message<> mousedragdelta { this, "mousedragdelta",
		MIN_FUNCTION {
			ui::target	t { args };
			number		x { args[2] };
			number		y { args[3] };
			int			modifiers { args[4] };
			number		factor { t.width() - 2.0 };	// how much precision (vs. immediacy) you have when dragging the knob
			number		delta;

			if (modifiers & c74::max::eCommandKey)
				factor = factor * 0.8;
			else if (modifiers & c74::max::eShiftKey)
				factor = factor * 50.0;
			factor = factor / m_range_delta;

			if (m_tracking == tracking::horizontal)
				delta = x;
			else if (m_tracking == tracking::vertical)
				delta = -y;
			else {						//  tracking::both
				if (fabs(x) > fabs(y))
					delta = x;
				else
					delta = -y;
			}

			m_anchor += delta / factor;
			m_anchor = MIN_CLAMP(m_anchor, m_range[0], m_range[1]);

			if (modifiers & c74::max::eCommandKey)
				m_number(static_cast<long>(m_anchor)); //a change in integer-steps
			else
				m_number(m_anchor);

			return {};
		}
	};

	message<> mousedoubleclick { this, "mousedoubleclick",
		MIN_FUNCTION {
			number val = m_default;
			m_number(val);
			m_anchor = val;
			return {};
		}
	};


	message<> paint { this, "paint",
		MIN_FUNCTION {
			using namespace ui;

			target t { args };

			auto value = (m_value - m_range[0]) / (m_range[1] - m_range[0]);
			auto pos =  ((t.width()-3) * value) + 1;	// one pixel for each border and -1 for counting to N-1

			rect<fill> {			// background
				t,
				color { {0.7, 0.7, 0.7, 1.0} },
			};

			rect<> {				// frame
				t,
				color { {0.3, 0.3, 0.3, 1.0} },
				line_width { 3.0 }
			};

			rect<fill> {			// active part of the slider
				t,
				color { {0.8, 0.6, 0.2, 1.0} },
				position { 1.0, 1.0 },
				size { pos, -2.0 }
			};

			rect<fill> {			// slider knob
				t,
				color { color::black },
				position { pos, 1.0 },
				size { 4.0, -2.0 }
			};

			text {					// text display
				t,
				color { color::white },
				position { m_offset[0], m_offset[1] + m_fontsize*0.5 },
				fontface { m_fontname },
				fontsize { m_fontsize },
				content { m_text }
			};

			return {};
		}
	};

private:
	number	m_unclipped_value {0.0};
	number	m_value;
	number	m_anchor;
	string	m_text;
	bool	m_mouseover {};
	number	m_mouse_position[2];
	number	m_range_delta {1.0};

	void update_text() {
		if (m_mouseover && m_showvalue) {
			symbol unit = m_unit;
			m_text = std::to_string(m_unclipped_value);
			m_text += " ";
			m_text += unit.c_str();
		}
		else {
			symbol label = m_label;
			m_text = label.c_str();
		}
		redraw();
	}
};


MIN_EXTERNAL(min_textslider);
