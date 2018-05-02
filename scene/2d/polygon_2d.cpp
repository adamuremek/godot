/*************************************************************************/
/*  polygon_2d.cpp                                                       */
/*************************************************************************/
/*                       This file is part of:                           */
/*                           GODOT ENGINE                                */
/*                      https://godotengine.org                          */
/*************************************************************************/
/* Copyright (c) 2007-2018 Juan Linietsky, Ariel Manzur.                 */
/* Copyright (c) 2014-2018 Godot Engine contributors (cf. AUTHORS.md)    */
/*                                                                       */
/* Permission is hereby granted, free of charge, to any person obtaining */
/* a copy of this software and associated documentation files (the       */
/* "Software"), to deal in the Software without restriction, including   */
/* without limitation the rights to use, copy, modify, merge, publish,   */
/* distribute, sublicense, and/or sell copies of the Software, and to    */
/* permit persons to whom the Software is furnished to do so, subject to */
/* the following conditions:                                             */
/*                                                                       */
/* The above copyright notice and this permission notice shall be        */
/* included in all copies or substantial portions of the Software.       */
/*                                                                       */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,       */
/* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF    */
/* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.*/
/* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY  */
/* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,  */
/* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE     */
/* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                */
/*************************************************************************/

#include "polygon_2d.h"
#include "core/math/geometry.h"

Dictionary Polygon2D::_edit_get_state() const {
	Dictionary state = Node2D::_edit_get_state();
	state["offset"] = offset;
	return state;
}

void Polygon2D::_edit_set_state(const Dictionary &p_state) {
	Node2D::_edit_set_state(p_state);
	set_offset(p_state["offset"]);
}

void Polygon2D::_edit_set_pivot(const Point2 &p_pivot) {
	set_position(get_transform().xform(p_pivot));
	set_offset(get_offset() - p_pivot);
}

Point2 Polygon2D::_edit_get_pivot() const {
	return Vector2();
}

bool Polygon2D::_edit_use_pivot() const {
	return true;
}

Rect2 Polygon2D::_edit_get_rect() const {
	if (rect_cache_dirty) {
		int l = polygon.size();
		PoolVector<Vector2>::Read r = polygon.read();
		item_rect = Rect2();
		for (int i = 0; i < l; i++) {
			Vector2 pos = r[i] + offset;
			if (i == 0)
				item_rect.position = pos;
			else
				item_rect.expand_to(pos);
		}
		rect_cache_dirty = false;
	}

	return item_rect;
}

bool Polygon2D::_edit_use_rect() const {
	return true;
}

bool Polygon2D::_edit_is_selected_on_click(const Point2 &p_point, double p_tolerance) const {

	return Geometry::is_point_in_polygon(p_point - get_offset(), Variant(polygon));
}

void Polygon2D::_notification(int p_what) {

	switch (p_what) {

		case NOTIFICATION_DRAW: {

			if (polygon.size() < 3)
				return;

			Vector<Vector2> points;
			Vector<Vector2> uvs;

			points.resize(polygon.size());

			int len = points.size();
			{

				PoolVector<Vector2>::Read polyr = polygon.read();
				for (int i = 0; i < len; i++) {
					points[i] = polyr[i] + offset;
				}
			}

			if (invert) {

				Rect2 bounds;
				int highest_idx = -1;
				float highest_y = -1e20;
				float sum = 0;

				for (int i = 0; i < len; i++) {
					if (i == 0)
						bounds.position = points[i];
					else
						bounds.expand_to(points[i]);
					if (points[i].y > highest_y) {
						highest_idx = i;
						highest_y = points[i].y;
					}
					int ni = (i + 1) % len;
					sum += (points[ni].x - points[i].x) * (points[ni].y + points[i].y);
				}

				bounds = bounds.grow(invert_border);

				Vector2 ep[7] = {
					Vector2(points[highest_idx].x, points[highest_idx].y + invert_border),
					Vector2(bounds.position + bounds.size),
					Vector2(bounds.position + Vector2(bounds.size.x, 0)),
					Vector2(bounds.position),
					Vector2(bounds.position + Vector2(0, bounds.size.y)),
					Vector2(points[highest_idx].x - CMP_EPSILON, points[highest_idx].y + invert_border),
					Vector2(points[highest_idx].x - CMP_EPSILON, points[highest_idx].y),
				};

				if (sum > 0) {
					SWAP(ep[1], ep[4]);
					SWAP(ep[2], ep[3]);
					SWAP(ep[5], ep[0]);
					SWAP(ep[6], points[highest_idx]);
				}

				points.resize(points.size() + 7);
				for (int i = points.size() - 1; i >= highest_idx + 7; i--) {

					points[i] = points[i - 7];
				}

				for (int i = 0; i < 7; i++) {

					points[highest_idx + i + 1] = ep[i];
				}

				len = points.size();
			}

			if (texture.is_valid()) {

				Transform2D texmat(tex_rot, tex_ofs);
				texmat.scale(tex_scale);
				Size2 tex_size = texture->get_size();
				uvs.resize(points.size());

				if (points.size() == uv.size()) {

					PoolVector<Vector2>::Read uvr = uv.read();

					for (int i = 0; i < len; i++) {
						uvs[i] = texmat.xform(uvr[i]) / tex_size;
					}

				} else {
					for (int i = 0; i < len; i++) {
						uvs[i] = texmat.xform(points[i]) / tex_size;
					}
				}
			}

			Vector<Color> colors;
			int color_len = vertex_colors.size();
			colors.resize(len);
			{
				PoolVector<Color>::Read color_r = vertex_colors.read();
				for (int i = 0; i < color_len && i < len; i++) {
					colors[i] = color_r[i];
				}
				for (int i = color_len; i < len; i++) {
					colors[i] = color;
				}
			}

			//			Vector<int> indices = Geometry::triangulate_polygon(points);
			//			VS::get_singleton()->canvas_item_add_triangle_array(get_canvas_item(), indices, points, colors, uvs, texture.is_valid() ? texture->get_rid() : RID());

			if (invert || splits.size() == 0) {
				VS::get_singleton()->canvas_item_add_polygon(get_canvas_item(), points, colors, uvs, texture.is_valid() ? texture->get_rid() : RID(), RID(), antialiased);
			} else {
				//use splits
				Vector<int> loop;
				int sc = splits.size();
				PoolVector<int>::Read r = splits.read();
				int last = points.size();

				Vector<Vector<int> > loops;

				for (int i = 0; i < last; i++) {

					int split;
					int min_end = -1;

					do {

						loop.push_back(i);

						split = -1;
						int end = -1;

						for (int j = 0; j < sc; j += 2) {
							if (r[j + 1] >= last)
								continue; //no longer valid
							if (min_end != -1 && r[j + 1] >= min_end)
								continue;
							if (r[j] == i) {
								if (split == -1 || r[j + 1] > end) {
									split = r[j];
									end = r[j + 1];
								}
							}
						}

						if (split != -1) {
							for (int j = end; j < last; j++) {
								loop.push_back(j);
							}
							loops.push_back(loop);
							last = end + 1;
							loop.clear();
							min_end = end; //avoid this split from repeating
						}

					} while (split != -1);
				}

				if (loop.size()) {
					loops.push_back(loop);
				}

				Vector<int> indices;

				for (int i = 0; i < loops.size(); i++) {
					Vector<int> loop = loops[i];
					Vector<Vector2> vertices;
					vertices.resize(loop.size());
					for (int j = 0; j < vertices.size(); j++) {
						vertices[j] = points[loop[j]];
					}
					Vector<int> sub_indices = Geometry::triangulate_polygon(vertices);
					int from = indices.size();
					indices.resize(from + sub_indices.size());
					for (int j = 0; j < sub_indices.size(); j++) {
						indices[from + j] = loop[sub_indices[j]];
					}
				}

				//print_line("loops: " + itos(loops.size()) + " indices: " + itos(indices.size()));

				VS::get_singleton()->canvas_item_add_triangle_array(get_canvas_item(), indices, points, colors, uvs, texture.is_valid() ? texture->get_rid() : RID());
			}

		} break;
	}
}

void Polygon2D::set_polygon(const PoolVector<Vector2> &p_polygon) {
	polygon = p_polygon;
	rect_cache_dirty = true;
	update();
}

PoolVector<Vector2> Polygon2D::get_polygon() const {

	return polygon;
}

void Polygon2D::set_uv(const PoolVector<Vector2> &p_uv) {

	uv = p_uv;
	update();
}

PoolVector<Vector2> Polygon2D::get_uv() const {

	return uv;
}

void Polygon2D::set_splits(const PoolVector<int> &p_splits) {

	ERR_FAIL_COND(p_splits.size() & 1); //splits should be multiple of 2
	splits = p_splits;
	update();
}

PoolVector<int> Polygon2D::get_splits() const {

	return splits;
}

void Polygon2D::set_color(const Color &p_color) {

	color = p_color;
	update();
}
Color Polygon2D::get_color() const {

	return color;
}

void Polygon2D::set_vertex_colors(const PoolVector<Color> &p_colors) {

	vertex_colors = p_colors;
	update();
}
PoolVector<Color> Polygon2D::get_vertex_colors() const {

	return vertex_colors;
}

void Polygon2D::set_texture(const Ref<Texture> &p_texture) {

	texture = p_texture;

	/*if (texture.is_valid()) {
		uint32_t flags=texture->get_flags();
		flags&=~Texture::FLAG_REPEAT;
		if (tex_tile)
			flags|=Texture::FLAG_REPEAT;

		texture->set_flags(flags);
	}*/
	update();
}
Ref<Texture> Polygon2D::get_texture() const {

	return texture;
}

void Polygon2D::set_texture_offset(const Vector2 &p_offset) {

	tex_ofs = p_offset;
	update();
}
Vector2 Polygon2D::get_texture_offset() const {

	return tex_ofs;
}

void Polygon2D::set_texture_rotation(float p_rot) {

	tex_rot = p_rot;
	update();
}
float Polygon2D::get_texture_rotation() const {

	return tex_rot;
}

void Polygon2D::set_texture_rotation_degrees(float p_rot) {

	set_texture_rotation(Math::deg2rad(p_rot));
}
float Polygon2D::get_texture_rotation_degrees() const {

	return Math::rad2deg(get_texture_rotation());
}

void Polygon2D::set_texture_scale(const Size2 &p_scale) {

	tex_scale = p_scale;
	update();
}
Size2 Polygon2D::get_texture_scale() const {

	return tex_scale;
}

void Polygon2D::set_invert(bool p_invert) {

	invert = p_invert;
	update();
}
bool Polygon2D::get_invert() const {

	return invert;
}

void Polygon2D::set_antialiased(bool p_antialiased) {

	antialiased = p_antialiased;
	update();
}
bool Polygon2D::get_antialiased() const {

	return antialiased;
}

void Polygon2D::set_invert_border(float p_invert_border) {

	invert_border = p_invert_border;
	update();
}
float Polygon2D::get_invert_border() const {

	return invert_border;
}

void Polygon2D::set_offset(const Vector2 &p_offset) {

	offset = p_offset;
	rect_cache_dirty = true;
	update();
	_change_notify("offset");
}

Vector2 Polygon2D::get_offset() const {

	return offset;
}

void Polygon2D::add_bone(const NodePath &p_path, const PoolVector<float> &p_weights) {

	Bone bone;
	bone.path = p_path;
	bone.weights = p_weights;
	bone_weights.push_back(bone);
}
int Polygon2D::get_bone_count() const {
	return bone_weights.size();
}
NodePath Polygon2D::get_bone_path(int p_index) const {
	ERR_FAIL_INDEX_V(p_index, bone_weights.size(), NodePath());
	return bone_weights[p_index].path;
}
PoolVector<float> Polygon2D::get_bone_weights(int p_index) const {

	ERR_FAIL_INDEX_V(p_index, bone_weights.size(), PoolVector<float>());
	return bone_weights[p_index].weights;
}
void Polygon2D::erase_bone(int p_idx) {

	ERR_FAIL_INDEX(p_idx, bone_weights.size());
	bone_weights.remove(p_idx);
}

void Polygon2D::clear_bones() {
	bone_weights.clear();
}

void Polygon2D::set_bone_weights(int p_index, const PoolVector<float> &p_weights) {
	ERR_FAIL_INDEX(p_index, bone_weights.size());
	bone_weights[p_index].weights = p_weights;
	update();
}
void Polygon2D::set_bone_path(int p_index, const NodePath &p_path) {
	ERR_FAIL_INDEX(p_index, bone_weights.size());
	bone_weights[p_index].path = p_path;
	update();
}

Array Polygon2D::_get_bones() const {
	Array bones;
	for (int i = 0; i < get_bone_count(); i++) {
		bones.push_back(get_bone_path(i));
		bones.push_back(get_bone_weights(i));
	}
	return bones;
}
void Polygon2D::_set_bones(const Array &p_bones) {

	ERR_FAIL_COND(p_bones.size() & 1);
	clear_bones();
	for (int i = 0; i < p_bones.size(); i += 2) {
		add_bone(p_bones[i], p_bones[i + 1]);
	}
}

void Polygon2D::set_skeleton(const NodePath &p_skeleton) {
	skeleton = p_skeleton;
	update();
}
NodePath Polygon2D::get_skeleton() const {
	return skeleton;
}

void Polygon2D::_bind_methods() {

	ClassDB::bind_method(D_METHOD("set_polygon", "polygon"), &Polygon2D::set_polygon);
	ClassDB::bind_method(D_METHOD("get_polygon"), &Polygon2D::get_polygon);

	ClassDB::bind_method(D_METHOD("set_uv", "uv"), &Polygon2D::set_uv);
	ClassDB::bind_method(D_METHOD("get_uv"), &Polygon2D::get_uv);

	ClassDB::bind_method(D_METHOD("set_color", "color"), &Polygon2D::set_color);
	ClassDB::bind_method(D_METHOD("get_color"), &Polygon2D::get_color);

	ClassDB::bind_method(D_METHOD("set_splits", "splits"), &Polygon2D::set_splits);
	ClassDB::bind_method(D_METHOD("get_splits"), &Polygon2D::get_splits);

	ClassDB::bind_method(D_METHOD("set_vertex_colors", "vertex_colors"), &Polygon2D::set_vertex_colors);
	ClassDB::bind_method(D_METHOD("get_vertex_colors"), &Polygon2D::get_vertex_colors);

	ClassDB::bind_method(D_METHOD("set_texture", "texture"), &Polygon2D::set_texture);
	ClassDB::bind_method(D_METHOD("get_texture"), &Polygon2D::get_texture);

	ClassDB::bind_method(D_METHOD("set_texture_offset", "texture_offset"), &Polygon2D::set_texture_offset);
	ClassDB::bind_method(D_METHOD("get_texture_offset"), &Polygon2D::get_texture_offset);

	ClassDB::bind_method(D_METHOD("set_texture_rotation", "texture_rotation"), &Polygon2D::set_texture_rotation);
	ClassDB::bind_method(D_METHOD("get_texture_rotation"), &Polygon2D::get_texture_rotation);

	ClassDB::bind_method(D_METHOD("set_texture_rotation_degrees", "texture_rotation"), &Polygon2D::set_texture_rotation_degrees);
	ClassDB::bind_method(D_METHOD("get_texture_rotation_degrees"), &Polygon2D::get_texture_rotation_degrees);

	ClassDB::bind_method(D_METHOD("set_texture_scale", "texture_scale"), &Polygon2D::set_texture_scale);
	ClassDB::bind_method(D_METHOD("get_texture_scale"), &Polygon2D::get_texture_scale);

	ClassDB::bind_method(D_METHOD("set_invert", "invert"), &Polygon2D::set_invert);
	ClassDB::bind_method(D_METHOD("get_invert"), &Polygon2D::get_invert);

	ClassDB::bind_method(D_METHOD("set_antialiased", "antialiased"), &Polygon2D::set_antialiased);
	ClassDB::bind_method(D_METHOD("get_antialiased"), &Polygon2D::get_antialiased);

	ClassDB::bind_method(D_METHOD("set_invert_border", "invert_border"), &Polygon2D::set_invert_border);
	ClassDB::bind_method(D_METHOD("get_invert_border"), &Polygon2D::get_invert_border);

	ClassDB::bind_method(D_METHOD("set_offset", "offset"), &Polygon2D::set_offset);
	ClassDB::bind_method(D_METHOD("get_offset"), &Polygon2D::get_offset);

	ClassDB::bind_method(D_METHOD("add_bone", "path", "weights"), &Polygon2D::add_bone);
	ClassDB::bind_method(D_METHOD("get_bone_count"), &Polygon2D::get_bone_count);
	ClassDB::bind_method(D_METHOD("get_bone_path", "index"), &Polygon2D::get_bone_path);
	ClassDB::bind_method(D_METHOD("get_bone_weights", "index"), &Polygon2D::get_bone_weights);
	ClassDB::bind_method(D_METHOD("erase_bone", "index"), &Polygon2D::erase_bone);
	ClassDB::bind_method(D_METHOD("clear_bones"), &Polygon2D::clear_bones);
	ClassDB::bind_method(D_METHOD("set_bone_path", "index", "path"), &Polygon2D::set_bone_path);
	ClassDB::bind_method(D_METHOD("set_bone_weights", "index", "weights"), &Polygon2D::set_bone_weights);

	ClassDB::bind_method(D_METHOD("set_skeleton", "skeleton"), &Polygon2D::set_skeleton);
	ClassDB::bind_method(D_METHOD("get_skeleton"), &Polygon2D::get_skeleton);

	ClassDB::bind_method(D_METHOD("_set_bones", "bones"), &Polygon2D::_set_bones);
	ClassDB::bind_method(D_METHOD("_get_bones"), &Polygon2D::_get_bones);

	ADD_PROPERTY(PropertyInfo(Variant::POOL_VECTOR2_ARRAY, "polygon"), "set_polygon", "get_polygon");
	ADD_PROPERTY(PropertyInfo(Variant::POOL_VECTOR2_ARRAY, "uv"), "set_uv", "get_uv");
	ADD_PROPERTY(PropertyInfo(Variant::POOL_INT_ARRAY, "splits"), "set_splits", "get_splits");
	ADD_PROPERTY(PropertyInfo(Variant::COLOR, "color"), "set_color", "get_color");
	ADD_PROPERTY(PropertyInfo(Variant::POOL_COLOR_ARRAY, "vertex_colors"), "set_vertex_colors", "get_vertex_colors");
	ADD_PROPERTY(PropertyInfo(Variant::VECTOR2, "offset"), "set_offset", "get_offset");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "antialiased"), "set_antialiased", "get_antialiased");
	ADD_GROUP("Texture", "");
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "texture", PROPERTY_HINT_RESOURCE_TYPE, "Texture"), "set_texture", "get_texture");
	ADD_GROUP("Texture", "texture_");
	ADD_PROPERTY(PropertyInfo(Variant::VECTOR2, "texture_offset"), "set_texture_offset", "get_texture_offset");
	ADD_PROPERTY(PropertyInfo(Variant::VECTOR2, "texture_scale"), "set_texture_scale", "get_texture_scale");
	ADD_PROPERTY(PropertyInfo(Variant::REAL, "texture_rotation_degrees", PROPERTY_HINT_RANGE, "-1440,1440,0.1"), "set_texture_rotation_degrees", "get_texture_rotation_degrees");
	ADD_PROPERTY(PropertyInfo(Variant::REAL, "texture_rotation", PROPERTY_HINT_NONE, "", 0), "set_texture_rotation", "get_texture_rotation");
	ADD_GROUP("Skeleton", "");
	ADD_PROPERTY(PropertyInfo(Variant::NODE_PATH, "skeleton"), "set_skeleton", "get_skeleton");

	ADD_GROUP("Invert", "invert_");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "invert_enable"), "set_invert", "get_invert");
	ADD_PROPERTY(PropertyInfo(Variant::REAL, "invert_border", PROPERTY_HINT_RANGE, "0.1,16384,0.1"), "set_invert_border", "get_invert_border");

	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "bones", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_NOEDITOR), "_set_bones", "_get_bones");
}

Polygon2D::Polygon2D() {

	invert = 0;
	invert_border = 100;
	antialiased = false;
	tex_rot = 0;
	tex_tile = true;
	tex_scale = Vector2(1, 1);
	color = Color(1, 1, 1);
	rect_cache_dirty = true;
}
