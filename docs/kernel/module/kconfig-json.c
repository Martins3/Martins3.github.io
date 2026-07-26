// SPDX-License-Identifier: GPL-2.0
/*
 * Export the parsed Kconfig model as JSON for external visualizers.
 *
 * This intentionally reuses the in-tree lkc parser/evaluator. The JSON is a
 * read-only snapshot of the menu tree, symbols, properties, and direct graph
 * edges; consumers should not try to reimplement Kconfig value calculation.
 */

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <list.h>
#include <xalloc.h>

#include "internal.h"
#include "lkc.h"

struct json_ctx {
	FILE *out;
	bool first_symbol;
	bool first_menu;
	bool first_edge;
	int next_menu_id;
};

static const char *tri_name(tristate tri)
{
	switch (tri) {
	case no:
		return "n";
	case mod:
		return "m";
	case yes:
		return "y";
	}

	return "?";
}

static int menu_id(const struct menu *menu)
{
	return (int)(intptr_t)menu->data;
}

static void json_string(FILE *out, const char *s)
{
	fputc('"', out);

	if (s) {
		for (; *s; s++) {
			unsigned char c = *s;

			switch (c) {
			case '"':
				fputs("\\\"", out);
				break;
			case '\\':
				fputs("\\\\", out);
				break;
			case '\b':
				fputs("\\b", out);
				break;
			case '\f':
				fputs("\\f", out);
				break;
			case '\n':
				fputs("\\n", out);
				break;
			case '\r':
				fputs("\\r", out);
				break;
			case '\t':
				fputs("\\t", out);
				break;
			default:
				if (c < 0x20)
					fprintf(out, "\\u%04x", c);
				else
					fputc(c, out);
				break;
			}
		}
	}

	fputc('"', out);
}

static void json_string_or_null(FILE *out, const char *s)
{
	if (s)
		json_string(out, s);
	else
		fputs("null", out);
}

static void json_expr_text(FILE *out, const struct expr *expr)
{
	struct gstr gs = str_new();

	expr_gstr_print(expr, &gs);
	json_string(out, str_get(&gs));
	str_free(&gs);
}

static const char *expr_op_name(enum expr_type type)
{
	switch (type) {
	case E_OR:
		return "or";
	case E_AND:
		return "and";
	case E_NOT:
		return "not";
	case E_EQUAL:
		return "equal";
	case E_UNEQUAL:
		return "unequal";
	case E_LTH:
		return "lt";
	case E_LEQ:
		return "le";
	case E_GTH:
		return "gt";
	case E_GEQ:
		return "ge";
	case E_RANGE:
		return "range";
	default:
		return "unknown";
	}
}

static void json_symbol_ref(FILE *out, const struct symbol *sym)
{
	fprintf(out, "{\"op\":\"symbol\",\"name\":");
	if (sym && sym->name)
		json_string(out, sym->name);
	else
		json_string(out, "<choice>");
	if (sym)
		fprintf(out, ",\"value\":\"%s\",\"type\":\"%s\"",
			sym_get_string_value((struct symbol *)sym), sym_type_name(sym->type));
	fprintf(out, "}");
}

static void json_expr_ast(FILE *out, const struct expr *expr)
{
	if (!expr) {
		fputs("{\"op\":\"const\",\"value\":\"y\"}", out);
		return;
	}

	switch (expr->type) {
	case E_SYMBOL:
		json_symbol_ref(out, expr->left.sym);
		break;
	case E_NOT:
		fprintf(out, "{\"op\":\"not\",\"arg\":");
		json_expr_ast(out, expr->left.expr);
		fputc('}', out);
		break;
	case E_OR:
	case E_AND:
		fprintf(out, "{\"op\":\"%s\",\"left\":", expr_op_name(expr->type));
		json_expr_ast(out, expr->left.expr);
		fputs(",\"right\":", out);
		json_expr_ast(out, expr->right.expr);
		fputc('}', out);
		break;
	case E_EQUAL:
	case E_UNEQUAL:
	case E_LTH:
	case E_LEQ:
	case E_GTH:
	case E_GEQ:
	case E_RANGE:
		fprintf(out, "{\"op\":\"%s\",\"left\":", expr_op_name(expr->type));
		json_symbol_ref(out, expr->left.sym);
		fputs(",\"right\":", out);
		json_symbol_ref(out, expr->right.sym);
		fputc('}', out);
		break;
	default:
		fprintf(out, "{\"op\":\"unknown\",\"type\":%d}", expr->type);
		break;
	}
}

static void json_expr_obj(FILE *out, const struct expr *expr)
{
	fputs("{\"text\":", out);
	json_expr_text(out, expr);
	fputs(",\"ast\":", out);
	json_expr_ast(out, expr);
	fputc('}', out);
}

static bool menu_has_any_prompt(const struct symbol *sym)
{
	struct menu *menu;

	list_for_each_entry(menu, &sym->menus, link) {
		if (menu->prompt)
			return true;
	}

	return false;
}

static void emit_location(FILE *out, const char *filename, int lineno)
{
	fputs("\"file\":", out);
	json_string_or_null(out, filename);
	fprintf(out, ",\"line\":%d", lineno);
}

static void emit_definitions(FILE *out, const struct symbol *sym)
{
	struct menu *menu;
	bool first = true;

	fputc('[', out);
	list_for_each_entry(menu, &sym->menus, link) {
		if (!first)
			fputc(',', out);
		first = false;

		fputc('{', out);
		emit_location(out, menu->filename, menu->lineno);
		fputs(",\"menuId\":", out);
		fprintf(out, "%d", menu_id(menu));
		fputs(",\"prompt\":", out);
		if (menu->prompt && menu->prompt->text)
			json_string(out, menu->prompt->text);
		else
			fputs("null", out);
		fputs(",\"depends\":", out);
		json_expr_obj(out, menu->dep);
		if (menu->prompt) {
			fputs(",\"visible\":", out);
			json_expr_obj(out, menu->prompt->visible.expr);
		}
		fputc('}', out);
	}
	fputc(']', out);
}

static void emit_prop_list(FILE *out, const struct symbol *sym, enum prop_type type)
{
	struct property *prop;
	bool first = true;

	fputc('[', out);
	for_all_properties(sym, prop, type) {
		if (!first)
			fputc(',', out);
		first = false;

		fputc('{', out);
		emit_location(out, prop->filename, prop->lineno);
		fputs(",\"expr\":", out);
		json_expr_obj(out, prop->expr);
		fputs(",\"condition\":", out);
		json_expr_obj(out, prop->visible.expr);
		if (type == P_SELECT || type == P_IMPLY) {
			struct symbol *target = prop_get_symbol(prop);

			fputs(",\"target\":", out);
			if (target && target->name)
				json_string(out, target->name);
			else
				fputs("null", out);
		}
		fputc('}', out);
	}
	fputc(']', out);
}

static void emit_symbol(struct json_ctx *ctx, struct symbol *sym)
{
	FILE *out = ctx->out;

	if (!sym || !sym->name)
		return;

	sym_calc_value(sym);

	if (!ctx->first_symbol)
		fputc(',', out);
	ctx->first_symbol = false;

	fprintf(out, "\n    {\"name\":");
	json_string(out, sym->name);
	fprintf(out, ",\"type\":\"%s\"", sym_type_name(sym_get_type(sym)));
	fprintf(out, ",\"value\":\"%s\"", sym_get_string_value(sym));
	fprintf(out, ",\"visible\":\"%s\"", tri_name(sym->visible));
	fprintf(out, ",\"userValue\":");
	if (sym_has_value(sym)) {
		if (sym->type == S_BOOLEAN || sym->type == S_TRISTATE)
			json_string(out, tri_name(sym->def[S_DEF_USER].tri));
		else
			json_string_or_null(out, sym->def[S_DEF_USER].val);
	} else {
		fputs("null", out);
	}
	fprintf(out, ",\"isChoice\":%s", sym_is_choice(sym) ? "true" : "false");
	fprintf(out, ",\"hasPrompt\":%s", menu_has_any_prompt(sym) ? "true" : "false");
	fputs(",\"definitions\":", out);
	emit_definitions(out, sym);
	fputs(",\"defaults\":", out);
	emit_prop_list(out, sym, P_DEFAULT);
	fputs(",\"selects\":", out);
	emit_prop_list(out, sym, P_SELECT);
	fputs(",\"implies\":", out);
	emit_prop_list(out, sym, P_IMPLY);
	fputc('}', out);
}

static const char *menu_type_name(enum menu_type type)
{
	switch (type) {
	case M_CHOICE:
		return "choice";
	case M_COMMENT:
		return "comment";
	case M_IF:
		return "if";
	case M_MENU:
		return "menu";
	case M_NORMAL:
		return "config";
	}

	return "unknown";
}

static void assign_menu_ids(struct json_ctx *ctx, struct menu *menu)
{
	struct menu *child;

	menu->data = (void *)(intptr_t)ctx->next_menu_id++;
	for (child = menu->list; child; child = child->next)
		assign_menu_ids(ctx, child);
}

static void emit_menu(struct json_ctx *ctx, struct menu *menu)
{
	FILE *out = ctx->out;
	struct menu *child;

	if (!ctx->first_menu)
		fputc(',', out);
	ctx->first_menu = false;

	fprintf(out, "\n    {\"id\":%d", menu_id(menu));
	fputs(",\"parentId\":", out);
	if (menu->parent)
		fprintf(out, "%d", menu_id(menu->parent));
	else
		fputs("null", out);
	fprintf(out, ",\"kind\":\"%s\"", menu_type_name(menu->type));
	fputs(",\"prompt\":", out);
	if (menu->prompt && menu->prompt->text)
		json_string(out, menu->prompt->text);
	else
		fputs("null", out);
	fputs(",\"symbol\":", out);
	if (menu->sym && menu->sym->name)
		json_string(out, menu->sym->name);
	else
		fputs("null", out);
	fputc(',', out);
	emit_location(out, menu->filename, menu->lineno);
	fputs(",\"depends\":", out);
	json_expr_obj(out, menu->dep);
	if (menu->visibility) {
		fputs(",\"visibility\":", out);
		json_expr_obj(out, menu->visibility);
	}
	fputc('}', out);

	for (child = menu->list; child; child = child->next)
		emit_menu(ctx, child);
}

static void emit_edge(struct json_ctx *ctx, const char *kind, const char *from,
		      const char *to, const struct expr *condition,
		      const char *filename, int lineno)
{
	FILE *out = ctx->out;

	if (!from || !to)
		return;

	if (!ctx->first_edge)
		fputc(',', out);
	ctx->first_edge = false;

	fprintf(out, "\n    {\"kind\":");
	json_string(out, kind);
	fputs(",\"from\":", out);
	json_string(out, from);
	fputs(",\"to\":", out);
	json_string(out, to);
	fputs(",\"condition\":", out);
	json_expr_obj(out, condition);
	fputc(',', out);
	emit_location(out, filename, lineno);
	fputc('}', out);
}

static void emit_expr_ref_edges(struct json_ctx *ctx, const char *kind,
				const char *from, const struct expr *expr,
				const char *filename, int lineno)
{
	if (!from || !expr)
		return;

	switch (expr->type) {
	case E_SYMBOL:
		if (expr->left.sym && expr->left.sym->name)
			emit_edge(ctx, kind, from, expr->left.sym->name, expr,
				  filename, lineno);
		break;
	case E_NOT:
		emit_expr_ref_edges(ctx, kind, from, expr->left.expr, filename,
				    lineno);
		break;
	case E_OR:
	case E_AND:
		emit_expr_ref_edges(ctx, kind, from, expr->left.expr, filename,
				    lineno);
		emit_expr_ref_edges(ctx, kind, from, expr->right.expr, filename,
				    lineno);
		break;
	case E_EQUAL:
	case E_UNEQUAL:
	case E_LTH:
	case E_LEQ:
	case E_GTH:
	case E_GEQ:
	case E_RANGE:
		if (expr->left.sym && expr->left.sym->name)
			emit_edge(ctx, kind, from, expr->left.sym->name, expr,
				  filename, lineno);
		if (expr->right.sym && expr->right.sym->name)
			emit_edge(ctx, kind, from, expr->right.sym->name, expr,
				  filename, lineno);
		break;
	default:
		break;
	}
}

static void emit_symbol_edges(struct json_ctx *ctx, struct symbol *sym)
{
	struct menu *menu;
	struct property *prop;

	if (!sym || !sym->name)
		return;

	list_for_each_entry(menu, &sym->menus, link) {
		emit_expr_ref_edges(ctx, "depends", sym->name, menu->dep,
				    menu->filename, menu->lineno);
		if (menu->prompt)
			emit_expr_ref_edges(ctx, "visible", sym->name,
					    menu->prompt->visible.expr,
					    menu->prompt->filename,
					    menu->prompt->lineno);
	}

	for_all_properties(sym, prop, P_DEFAULT) {
		emit_expr_ref_edges(ctx, "default", sym->name, prop->expr,
				    prop->filename, prop->lineno);
		emit_expr_ref_edges(ctx, "default_if", sym->name,
				    prop->visible.expr, prop->filename,
				    prop->lineno);
	}

	for_all_properties(sym, prop, P_SELECT) {
		struct symbol *target = prop_get_symbol(prop);

		if (target && target->name)
			emit_edge(ctx, "select", sym->name, target->name,
				  prop->visible.expr, prop->filename,
				  prop->lineno);
	}

	for_all_properties(sym, prop, P_IMPLY) {
		struct symbol *target = prop_get_symbol(prop);

		if (target && target->name)
			emit_edge(ctx, "imply", sym->name, target->name,
				  prop->visible.expr, prop->filename,
				  prop->lineno);
	}
}

static void emit_menu_edges(struct json_ctx *ctx, struct menu *menu)
{
	struct menu *child;
	char from[32], to[32];

	for (child = menu->list; child; child = child->next) {
		snprintf(from, sizeof(from), "menu:%d", menu_id(menu));
		snprintf(to, sizeof(to), "menu:%d", menu_id(child));
		emit_edge(ctx, "menu_contains", from, to, child->dep,
			  child->filename, child->lineno);
		emit_menu_edges(ctx, child);
	}
}

int main(int argc, char **argv)
{
	struct json_ctx ctx = {
		.out = stdout,
		.first_symbol = true,
		.first_menu = true,
		.first_edge = true,
		.next_menu_id = 0,
	};
	struct symbol *sym;

	if (argc != 2) {
		fprintf(stderr, "usage: %s <Kconfig>\n", argv[0]);
		return 1;
	}

	conf_set_message_callback(NULL);
	conf_parse(argv[1]);
	conf_read(NULL);

	assign_menu_ids(&ctx, &rootmenu);

	fputs("{\n  \"schemaVersion\":1,\n  \"symbols\":[", stdout);
	for_all_symbols(sym)
		emit_symbol(&ctx, sym);

	fputs("\n  ],\n  \"menus\":[", stdout);
	emit_menu(&ctx, &rootmenu);

	fputs("\n  ],\n  \"edges\":[", stdout);
	ctx.first_edge = true;
	for_all_symbols(sym)
		emit_symbol_edges(&ctx, sym);
	emit_menu_edges(&ctx, &rootmenu);

	fputs("\n  ]\n}\n", stdout);

	return conf_errors() ? 1 : 0;
}
