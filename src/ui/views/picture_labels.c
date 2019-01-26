﻿/* ***************************************************************************
 * picture_label.c -- picture label view
 *
 * Copyright (C) 2018 by Liu Chao <lc-soft@live.cn>
 *
 * This file is part of the LC-Finder project, and may only be used, modified,
 * and distributed under the terms of the GPLv2.
 *
 * By continuing to use, modify, or distribute this file you indicate that you
 * have read the license and understand and accept it fully.
 *
 * The LC-Finder project is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE. See the GPL v2 for more details.
 *
 * You should have received a copy of the GPLv2 along with this file. It is
 * usually in the LICENSE.TXT file, If not, see <http://www.gnu.org/licenses/>.
 * ****************************************************************************/

/* ****************************************************************************
 * picture_label.c -- "图片标记" 视图
 *
 * 版权所有 (C) 2018 归属于 刘超 <lc-soft@live.cn>
 *
 * 这个文件是 LC-Finder 项目的一部分，并且只可以根据GPLv2许可协议来使用、更改和
 * 发布。
 *
 * 继续使用、修改或发布本文件，表明您已经阅读并完全理解和接受这个许可协议。
 *
 * LC-Finder 项目是基于使用目的而加以散布的，但不负任何担保责任，甚至没有适销
 * 性或特定用途的隐含担保，详情请参照GPLv2许可协议。
 *
 * 您应已收到附随于本文件的GPLv2许可协议的副本，它通常在 LICENSE 文件中，如果
 * 没有，请查看：<http://www.gnu.org/licenses/>.
 * ****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "finder.h"
#include "ui.h"
#include "i18n.h"
#include "file_storage.h"
#include <LCUI/gui/widget.h>
#include <LCUI/gui/widget/textview.h>
#include "labelbox.h"
#include "labelitem.h"
#include "picture.h"

typedef struct BoundingBoxItemRec_ {
	LCUI_Widget box;
	LCUI_Widget item;
	LCUI_RectF rect;
	LinkedListNode node;
} BoundingBoxItemRec, *BoundingBoxItem;

static struct PictureLabelsPanel {
	LCUI_Widget panel;
	LCUI_Widget labels;
	LCUI_Widget available_labels;
	PictureLabelsViewContextRec ctx;
	LinkedList labels_trending;
	LinkedList boxes;
} view;

#define GetWidget LCUIWidget_GetById

static void RenderAvailableLabels(void);

static void LabelsTrending_Add(DB_Tag tag)
{
	DB_Tag t;
	LinkedListNode *node;
	LinkedList *list = &view.labels_trending;

	for (LinkedList_Each(node, list)) {
		t = node->data;
		if (t->id == tag->id) {
			LinkedList_Unlink(list, node);
			LinkedList_InsertNode(list, 0, node);
			return;
		}
	}
	LinkedList_Append(list, DBTag_Dup(tag));
}

void LabelsPanel_UpdateBox(BoundingBoxItem data)
{
	LCUI_RectF rect;
	PictureLabelsViewContext ctx = &view.ctx;
	float width = ctx->scale * ctx->width;
	float height = ctx->scale * ctx->height;

	rect.width = data->rect.width * width;
	rect.height = data->rect.height * height;
	rect.x = data->rect.x * width - (ctx->focus_x - ctx->offset_x);
	rect.y = data->rect.y * height - (ctx->focus_y - ctx->offset_y);
	LabelBox_SetRect(data->box, &rect);
	if (ctx->width < 1 || ctx->height < 1) {
		Widget_Hide(data->box);
	} else {
		Widget_Show(data->box);
	}
}

void LabelsPanel_UpdateBoxes(void)
{
	LinkedListNode *node;

	for (LinkedList_Each(node, &view.boxes)) {
		LabelsPanel_UpdateBox(node->data);
	}
}

static void SetRandomRect(BoundingBoxItem data)
{
	LCUI_Rect img_rect;
	PictureLabelsViewContext ctx = &view.ctx;
	float width = ctx->scale * ctx->width;
	float height = ctx->scale * ctx->height;

	data->rect.x = 0.1f + (rand() % 10) * 0.04f;
	data->rect.y = 0.1f + (rand() % 10) * 0.04f;
	data->rect.width = 0.3f + (rand() % 10) * 0.02f;
	data->rect.height = 0.3f + (rand() % 10) * 0.02f;
	LCUIRectF_ValidateArea(&data->rect, 1.0f, 1.0f);
	img_rect.x = iround(data->rect.x * width);
	img_rect.y = iround(data->rect.y * height);
	img_rect.width = iround(data->rect.width * width);
	img_rect.height = iround(data->rect.height * height);
	LabelItem_SetRect(data->item, &img_rect);
	LabelsPanel_UpdateBox(data);
}

static void OnLabelBoxUpdate(LCUI_Widget w, LCUI_WidgetEvent e, void *arg)
{
	LCUI_RectF rect;
	LCUI_Rect img_rect;
	BoundingBoxItem data = e->data;
	PictureLabelsViewContext ctx = &view.ctx;
	float width = ctx->scale * ctx->width;
	float height = ctx->scale * ctx->height;

	LabelBox_GetRect(data->box, &rect);
	data->rect.x = (rect.x + ctx->focus_x - ctx->offset_x) / width;
	data->rect.y = (rect.y + ctx->focus_y - ctx->offset_y) / height;
	data->rect.width = rect.width / width;
	data->rect.height = rect.height / height;
	if (LCUIRectF_ValidateArea(&data->rect, 1.0f, 1.0f)) {
		LabelsPanel_UpdateBox(data);
	}
	img_rect.x = iround(data->rect.x * width);
	img_rect.y = iround(data->rect.y * height);
	img_rect.width = iround(data->rect.width * width);
	img_rect.height = iround(data->rect.height * height);
	LabelItem_SetNameW(data->item, LabelBox_GetNameW(data->box));
	LabelItem_SetRect(data->item, &img_rect);
}

static void OnLabelItemDestroy(LCUI_Widget w, LCUI_WidgetEvent e, void *arg)
{
	BoundingBoxItem data = e->data;

	LinkedList_Unlink(&view.boxes, &data->node);
	Widget_Destroy(data->box);
	free(data);
}

BoundingBoxItem LabelsPanel_AddBox(void)
{
	BoundingBoxItem data;

	if (!view.ctx.view) {
		return NULL;
	}

	data = malloc(sizeof(BoundingBoxItemRec));
	if (!data) {
		return NULL;
	}

	data->box = LCUIWidget_New("labelbox");
	data->item = LCUIWidget_New("labelitem");
	data->node.data = data;

	Widget_BindEvent(data->box, "change", OnLabelBoxUpdate, data, NULL);
	Widget_BindEvent(data->item, "destroy", OnLabelItemDestroy, data, NULL);
	Widget_Append(view.ctx.view, data->box);
	Widget_Append(view.labels, data->item);
	LinkedList_AppendNode(&view.boxes, &data->node);
	return data;
}

static void LabelsPanel_ClearBoxes(void)
{
	BoundingBoxItem data;
	LinkedListNode *node;

	for (LinkedList_Each(node, &view.boxes)) {
		data = node->data;
		Widget_Destroy(data->box);
	}
	LinkedList_Clear(&view.boxes, free);
}

static void LabelsPanel_LoadBoxes(void)
{
}

static void OnAddLabel(LCUI_Widget w, LCUI_WidgetEvent e, void *arg)
{
	BoundingBoxItem data;
	PictureLabelsViewContext ctx = &view.ctx;
	float width = ctx->scale * ctx->width;
	float height = ctx->scale * ctx->height;

	data = LabelsPanel_AddBox();
	if (!data) {
		return;
	}
	SetRandomRect(data);
	LabelItem_SetNameW(data->item, LabelBox_GetNameW(data->box));
}

static void OnHideView(LCUI_Widget w, LCUI_WidgetEvent e, void *arg)
{
	PictureView_HideLabels();
}

static void OnLabelClick(LCUI_Widget w, LCUI_WidgetEvent e, void *arg)
{
	BoundingBoxItem data;
	DB_Tag tag = e->data;
	wchar_t *name = DecodeUTF8(tag->name);

	LabelsTrending_Add(tag);
	data = LabelsPanel_AddBox();
	SetRandomRect(data);
	LabelBox_SetNameW(data->box, name);
	LabelItem_SetNameW(data->item, name);
	free(name);
	RenderAvailableLabels();
}

static void OnLabelEventDestroy(void *data)
{
	DBTag_Release(data);
}

static LCUI_Widget CreateLabel(DB_Tag tag)
{
	LCUI_Widget label;

	tag = DBTag_Dup(tag);
	label = LCUIWidget_New("textview");
	Widget_AddClass(label, "label");
	Widget_BindEvent(label, "click", OnLabelClick, tag,
			 OnLabelEventDestroy);
	TextView_SetText(label, tag->name);
	return label;
}

static void RenderAvailableLabels(void)
{
	size_t i;
	DB_Tag tag;
	LCUI_BOOL found;
	LinkedListNode *node;

	Widget_Empty(view.available_labels);
	for (LinkedList_Each(node, &view.labels_trending)) {
		Widget_Append(view.available_labels, CreateLabel(node->data));
	}
	for (i = 0; i < finder.n_tags; ++i) {
		found = FALSE;
		for (LinkedList_Each(node, &view.labels_trending)) {
			tag = node->data;
			if (tag->id == finder.tags[i]->id) {
				found = TRUE;
				break;
			}
		}
		if (!found) {
			Widget_Append(view.available_labels,
				      CreateLabel(finder.tags[i]));
		}
	}
}

void PictureView_InitLabels(void)
{
	LCUI_Widget btn_add, btn_hide;

	LinkedList_Init(&view.boxes);
	LinkedList_Init(&view.labels_trending);
	view.panel = GetWidget(ID_PANEL_PICTURE_LABELS);
	view.labels = GetWidget(ID_VIEW_PICTURE_LABELS);
	view.available_labels = GetWidget(ID_VIEW_PICTURE_AVAIL_LABELS);
	btn_add = GetWidget(ID_BTN_ADD_PICTURE_LABEL);
	btn_hide = GetWidget(ID_BTN_HIDE_PICTURE_LABEL);
	Widget_BindEvent(btn_add, "click", OnAddLabel, NULL, NULL);
	Widget_BindEvent(btn_hide, "click", OnHideView, NULL, NULL);
	Widget_Hide(view.panel);
	RenderAvailableLabels();
}

void PictureView_SetLabelsContext(PictureLabelsViewContext ctx)
{
	wchar_t *file = view.ctx.file;

	if (!file || wcscmp(file, ctx->file) != 0) {
		free(file);
		file = wcsdup2(ctx->file);
		LabelsPanel_ClearBoxes();
		LabelsPanel_LoadBoxes();
	}
	view.ctx = *ctx;
	view.ctx.file = file;
	LabelsPanel_UpdateBoxes();
}

void PictureView_ShowLabels(void)
{
	Widget_Show(view.panel);
	Widget_AddClass(view.panel->parent, "has-panel");
}

void PictureView_HideLabels(void)
{
	Widget_Hide(view.panel);
	Widget_RemoveClass(view.panel->parent, "has-panel");
}
