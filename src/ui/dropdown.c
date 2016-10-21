﻿/* ***************************************************************************
 * dropdown.c -- contextual menu for displaying options of target
 *
 * Copyright (C) 2016 by Liu Chao <lc-soft@live.cn>
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
 * dropdown.c -- 上下文菜单，用于显示目标的选项列表
 *
 * 版权所有 (C) 2016 归属于 刘超 <lc-soft@live.cn>
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

#include "finder.h"
#include <LCUI/gui/widget.h>
#include <LCUI/gui/widget/textview.h>
#include "dropdown.h"

typedef struct DropdownRec_ {
	LCUI_Widget target;
} DropdownRec, *Dropdown;

typedef struct DropdownItemRec_ {
	char *value;
	char *text;
} DropdownItemRec, *DropdownItem;

static struct DropdwonModule {
	LCUI_WidgetPrototype dropdown;
	LCUI_WidgetPrototype item;
	int change_event;
} self;

static char *dropdown_css = ToString(

dropdown {
	position: absolute;
	margin-top: 4px;
	padding: 10px 0;
	min-width: 140px;
	background-color: #fff;
	border: 1px solid #ccc;
	box-shadow: 0 6px 12px rgba(0,0,0,.175);
	z-index: 1000;
}
dropdown dropdown-item {
	width: 100%;
	padding: 5px 10px;
	line-height: 24px;
	display: block;
}
dropdown dropdown-item:hover {
	background-color: #eee;
}
dropdown dropdown-item:active {
	background-color: #ddd;
}

);

static void Dropdown_OnClickOther( LCUI_Widget w, 
				   LCUI_WidgetEvent e, void *arg )
{
	Dropdown data = Widget_GetData( e->data, self.dropdown );
	Widget_Hide( e->data );
	if( data->target ) {
		Widget_RemoveClass( data->target, "active" );
	}
}

static void Dropdown_Init( LCUI_Widget w )
{
	const size_t data_size = sizeof( DropdownRec );
	Dropdown data = Widget_AddData( w, self.dropdown, data_size );
	data->target = NULL;
	Widget_Hide( w );
	Widget_BindEvent( LCUIWidget_GetRoot(), "click", 
			  Dropdown_OnClickOther, w, NULL );
}

static void DropdownItem_Init( LCUI_Widget w )
{
	const size_t data_size = sizeof( DropdownItemRec );
	DropdownItem data = Widget_AddData( w, self.item, data_size );
	data->value = data->text = NULL;
	self.item->proto->init( w );
}

static void DropdownItem_Destrtoy( LCUI_Widget w )
{
	DropdownItem item = Widget_GetData( w, self.item );
	free( item->value );
	free( item->text );
	item->value = NULL;
	item->text = NULL;
}

void DropdownItem_SetData( LCUI_Widget w, const char *value,
			   const char *text )
{
	DropdownItem data = Widget_GetData( w, self.item );
	data->value = value ? strdup( value ) : NULL;
	data->text = text ? strdup( text ) : NULL;
	TextView_SetText( w, text );
}

static void DropdownItem_OnClick( LCUI_Widget w, LCUI_WidgetEvent e, void *arg )
{
	DropdownItem item;
	LCUI_Widget menu = e->data;
	LCUI_WidgetEventRec ev = {0};
	e->cancel_bubble = TRUE;
	ev.type = self.change_event;
	item = Widget_GetData( w, self.item );
	Widget_TriggerEvent( menu, &ev, item->value );
	Widget_Hide( menu );
}

void Dropdown_Toggle( LCUI_Widget w )
{
	Dropdown data = Widget_GetData( w, self.dropdown );
	LCUI_BOOL visible = !w->computed_style.visible;

	if( visible ) {
		Widget_Show( w );
		if( data->target ) {
			Widget_AddClass( data->target, "active" );
		}
	} else {
		Widget_Hide( w );
		if( data->target ) {
			Widget_RemoveClass( data->target, "active" );
		}
	}
	if( data->target ) {
		int x, y;
		Widget_GetAbsXY( data->target, w->parent, &x, &y );
		y += data->target->height;
		if( y + w->height > w->parent->height ) {
			Widget_GetAbsXY( data->target, w->parent, &x, &y );
			y = y - data->target->height - w->height;
		}
		Widget_Move( w, x, y );
	}
}

static void Dropdown_OnClick( LCUI_Widget w, LCUI_WidgetEvent e, void *arg )
{
	Dropdown_Toggle( e->data );
	e->cancel_bubble = TRUE;
}

void Dropdown_BindTarget( LCUI_Widget w, LCUI_Widget target )
{
	Dropdown data = Widget_GetData( w, self.dropdown );
	data->target = target;
	if( data->target ) {
		Widget_UnbindEvent( target, "click", Dropdown_OnClick );
	}
	Widget_BindEvent( target, "click", Dropdown_OnClick, w, NULL );
	data->target = target;
}

static void Dropdown_SetAttr( LCUI_Widget w, const char *name,
			      const char *value )
{
	if( strcasecmp( name, "data-target" ) == 0 ) {
		LCUI_Widget target = LCUIWidget_GetById( value );
		if( target ) {
			Dropdown_BindTarget( w, target );
		}
	}
}

LCUI_Widget Dropdwon_AddItem( LCUI_Widget w, const char *value,
			      const char *text )
{
	LCUI_Widget item = LCUIWidget_New( "dropdown-item" );
	DropdownItem_SetData( item, value, text );
	Widget_BindEvent( item, "click", DropdownItem_OnClick, w, NULL );
	Widget_Append( w, item );
	return item;
}

void LCUIWidget_AddDropdown( void )
{
	self.dropdown = LCUIWidget_NewPrototype( "dropdown", NULL );
	self.item = LCUIWidget_NewPrototype( "dropdown-item", "textview" );
	self.dropdown->init = Dropdown_Init;
	self.dropdown->setattr = Dropdown_SetAttr;
	self.item->init = DropdownItem_Init;
	self.item->destroy = DropdownItem_Destrtoy;
	self.change_event = LCUIWidget_AllocEventId();
	LCUIWidget_SetEventName( self.change_event, "change.dropdown" );
	LCUI_LoadCSSString( dropdown_css, NULL );
}