## 核心结构体
```c
/*
 * Represents a node in the menu tree, as seen in e.g. menuconfig (though used
 * for all front ends). Each symbol, menu, etc. defined in the Kconfig files
 * gets a node. A symbol defined in multiple locations gets one node at each
 * location.
 *
 * @choice_members: list of choice members with priority.
 */
struct menu {
	/* The next menu node at the same level */
	struct menu *next;

	/* The parent menu node, corresponding to e.g. a menu or choice */
	struct menu *parent;

	/* The first child menu node, for e.g. menus and choices */
	struct menu *list;

	/*
	 * The symbol associated with the menu node. Choices are implemented as
	 * a special kind of symbol. NULL for menus, comments, and ifs.
	 */
	struct symbol *sym;

	struct list_head link;	/* link to symbol::menus */

	struct list_head choice_members;

	/*
	 * The prompt associated with the node. This holds the prompt for a
	 * symbol as well as the text for a menu or comment, along with the
	 * type (P_PROMPT, P_MENU, etc.)
	 */
	struct property *prompt;

	/*
	 * 'visible if' dependencies. If more than one is given, they will be
	 * ANDed together.
	 */
	struct expr *visibility;

	/*
	 * Ordinary dependencies from e.g. 'depends on' and 'if', ANDed
	 * together
	 */
	struct expr *dep;

	/* MENU_* flags */
	unsigned int flags;

	/* Any help text associated with the node */
	char *help;

	/* The location where the menu node appears in the Kconfig files */
	const char *filename;
	int lineno;

	/* For use by front ends that need to store auxiliary data */
	void *data;
};
```

## 我们需要的
1. 依赖 sunrpc 的是什么?
2. 审核 localmodconfig 的配置
  - 在 def 中仅仅去配置那些 visible 的 config ，不要去配置那些非 visible 的 config
3. 图形化的展示

## seabios 直接把内核中的 kconfig 拷贝过来了

<script src="https://giscus.app/client.js"
        data-repo="martins3/martins3.github.io"
        data-repo-id="MDEwOlJlcG9zaXRvcnkyOTc4MjA0MDg="
        data-category="Show and tell"
        data-category-id="MDE4OkRpc2N1c3Npb25DYXRlZ29yeTMyMDMzNjY4"
        data-mapping="pathname"
        data-reactions-enabled="1"
        data-emit-metadata="0"
        data-theme="light"
        data-lang="zh-CN"
        crossorigin="anonymous"
        async>
</script>

本站所有文章转发 **CSDN** 将按侵权追究法律责任，其它情况随意。
