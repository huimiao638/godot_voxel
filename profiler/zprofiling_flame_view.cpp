#include "zprofiling_flame_view.h"
#include "zprofiler.h"
#include "zprofiling_client.h"

ZProfilingFlameView::ZProfilingFlameView() {
	set_clip_contents(true);
}

void ZProfilingFlameView::set_client(const ZProfilingClient *client) {
	_client = client;
}

void ZProfilingFlameView::set_thread(int thread_index) {
	if (thread_index != _thread_index) {
		_thread_index = thread_index;
		update();
	}
}

void ZProfilingFlameView::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_DRAW:
			_draw();
			break;

		default:
			break;
	}
}

void ZProfilingFlameView::_gui_input(Ref<InputEvent> p_event) {
}

static void draw_shaded_text(CanvasItem *ci, Ref<Font> font, Vector2 pos, String text, Color fg, Color bg) {
	ci->draw_string(font, pos + Vector2(1, 1), text, bg);
	ci->draw_string(font, pos, text, fg);
}

void ZProfilingFlameView::_draw() {
	VOXEL_PROFILE_SCOPE();

	const Color item_color(1.f, 0.5f, 0.f);
	const Color bg_color(0.f, 0.f, 0.f, 0.7f);
	const Color item_text_color(0.f, 0.f, 0.f);
	const Color text_fg_color(1.f, 1.f, 1.f);
	const Color text_bg_color(0.f, 0.f, 0.f);
	const int lane_separation = 1;
	const int lane_height = 20;
	const Vector2 text_margin(4, 4);

	// Background
	const Rect2 control_rect = get_rect();
	draw_rect(Rect2(0, 0, control_rect.size.x, control_rect.size.y), bg_color);

	Ref<Font> font = get_font("font");
	ERR_FAIL_COND(font.is_null());

	ERR_FAIL_COND(_client == nullptr);
	if (_thread_index >= _client->get_thread_count()) {
		return;
	}
	const ZProfilingClient::ThreadData &thread_data = _client->get_thread_data(_thread_index);
	if (thread_data.selected_frame == -1) {
		return;
	}
	const ZProfilingClient::Frame &frame = thread_data.frames[thread_data.selected_frame];

	if (frame.end_time == -1) {
		// Frame is not finalized
		return;
	}

	Rect2 item_rect;
	item_rect.position.y = lane_separation;
	item_rect.size.y = lane_height;

	const int frame_duration = frame.end_time - frame.begin_time;
	const float frame_duration_inv = 1.f / frame_duration;

	// Lanes and items
	for (int lane_index = 0; lane_index < frame.lanes.size(); ++lane_index) {
		const ZProfilingClient::Lane &lane = frame.lanes[lane_index];

		for (int item_index = 0; item_index < lane.items.size(); ++item_index) {
			const ZProfilingClient::Item &item = lane.items[item_index];

			// TODO Cull items
			// TODO Pan and zoom

			item_rect.position.x = control_rect.size.x * static_cast<float>(item.begin_time - frame.begin_time) * frame_duration_inv;
			item_rect.size.x = control_rect.size.x * static_cast<float>(item.end_time - item.begin_time) * frame_duration_inv;

			if (item_rect.size.x < 1.01f) {
				// Clamp item size so it doesn't disappear
				item_rect.size.x = 1.01f;

			} else if (item_rect.size.x > 1.f) {
				// Give a small gap to see glued items distinctly
				item_rect.size.x -= 1.f;
			}

			draw_rect(item_rect, item_color);

			if (item_rect.size.x > 100) {
				const String text = _client->get_string(item.description_id);
				//const Vector2 text_size = font->get_string_size(text);
				const Vector2 text_pos(
						item_rect.position.x + text_margin.x,
						item_rect.position.y + text_margin.y + font->get_ascent());
				draw_string(font, text_pos, text, item_text_color, item_rect.size.x);
			}

			// TODO Draw item text
		}

		item_rect.position.y += lane_height + lane_separation;
	}

	// Time graduations

	const float min_time_ms = 0.f;
	const float max_time_ms = (frame.end_time - frame.begin_time) / 1000.f;

	const String begin_time_text = String("{0} ms").format(varray(min_time_ms));
	const String end_time_text = String("{0} ms").format(varray(max_time_ms));

	const Vector2 min_text_pos(4, control_rect.size.y - font->get_height() + font->get_ascent());
	const Vector2 max_text_size = font->get_string_size(end_time_text);
	const Vector2 max_text_pos(control_rect.size.x - 4 - max_text_size.x, control_rect.size.y - font->get_height() + font->get_ascent());

	draw_shaded_text(this, font, min_text_pos, begin_time_text, text_fg_color, text_bg_color);
	draw_shaded_text(this, font, max_text_pos, end_time_text, text_fg_color, text_bg_color);
}

void ZProfilingFlameView::_bind_methods() {
	ClassDB::bind_method("_gui_input", &ZProfilingFlameView::_gui_input);
}
