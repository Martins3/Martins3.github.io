(() => {
	"use strict";
	const data = window.MATH_ATLAS;
	const domains = new Map(data.domains.map(domain => [domain.id, domain]));
	const nodes = new Map(data.nodes.map(node => [node.id, node]));
	const map = document.querySelector("#map");
	const detail = document.querySelector("#detail");
	const tooltip = document.querySelector("#tooltip");
	const svg = document.querySelector("#connections");
	const buttons = new Map();
	const positions = new Map();
	const courses = new Map(data.courses.map(course => [course.id, course]));
	const overviewIds = new Set(data.overviewIds);
	const catalog = document.querySelector("#curriculum");
	const dialog = document.querySelector("#detail-dialog");
	const mobile = window.matchMedia("(max-width: 760px)");
	const chapterCount = data.courses.reduce((sum, course) => sum + course.chapters.length, 0);
	const conceptCount = data.nodes.filter(node => node.course).length;
	let domainFilter = "all";
	let selected = null;
	let preview = null;
	let tooltipOwner = null;

	function element(tag, className, text) {
		const result = document.createElement(tag);
		if (className) result.className = className;
		if (text !== undefined) result.textContent = text;
		return result;
	}

	function hideTooltip() {
		tooltip.hidden = true;
		tooltipOwner?.removeAttribute("aria-describedby");
		tooltipOwner = null;
		preview = null;
		highlight(selected);
	}

	function showTooltip(node, button) {
		if (mobile.matches) return;
		tooltipOwner?.removeAttribute("aria-describedby");
		tooltipOwner = button;
		button.setAttribute("aria-describedby", "tooltip");
		tooltip.replaceChildren(element("strong", "", node.title), element("p", "", node.summary), element("small", "", "点击阅读详细解释与关联概念"));
		tooltip.hidden = false;
		const rect = button.getBoundingClientRect();
		const box = tooltip.getBoundingClientRect();
		tooltip.style.left = `${Math.max(12, Math.min(rect.left, window.innerWidth - box.width - 12))}px`;
		tooltip.style.top = `${Math.max(12, rect.bottom + box.height + 12 < window.innerHeight ? rect.bottom + 10 : rect.top - box.height - 10)}px`;
		preview = node.id;
		highlight(preview);
	}

	function highlight(id) {
		map.classList.toggle("has-selection", Boolean(id));
		const neighbours = new Set();
		if (id) {
			map.style.setProperty("--selected-color", domains.get(nodes.get(id).domain).color);
			for (const [source, target] of data.edges) {
				if (source === id) neighbours.add(target);
				if (target === id) neighbours.add(source);
			}
		}
		for (const [key, button] of buttons) {
			button.classList.toggle("selected", key === selected);
			button.classList.toggle("related", neighbours.has(key) || key === id);
			button.setAttribute("aria-pressed", String(key === selected));
		}
		for (const card of catalog.querySelectorAll(".course-card")) {
			card.classList.toggle("active-course", Boolean(id && nodes.get(id).course === card.dataset.course));
		}
		for (const path of svg.querySelectorAll(".edge")) {
			const active = path.dataset.source === id || path.dataset.target === id;
			path.classList.toggle("active", active);
			path.style.display = active || path.dataset.overview === "true" ? "" : "none";
		}
	}

	function openNode(id, { updateUrl = true, locate = false } = {}) {
		if (!nodes.has(id)) return;
		const target = nodes.get(id);
		if (target.course && domainFilter !== "all" && domainFilter !== target.domain) filterDomain("all");
		selected = id;
		hideTooltip();
		const node = nodes.get(id);
		const domain = domains.get(node.domain);
		detail.style.setProperty("--color", domain.color);
		detail.replaceChildren();
		const tag = element("div", "detail-tag");
		tag.append(element("span", "", `${domain.name} / CONCEPT`), element("span", "", node.level));
		const title = element("h2", "", node.title);
		title.tabIndex = -1;
		detail.append(tag, title);
		if (node.course) {
			const breadcrumb = element("button", "breadcrumb", `${courses.get(node.course).title} / ${node.chapter} ↗`);
			breadcrumb.addEventListener("click", () => { if (dialog.open) dialog.close(); locateNode(node.id); });
			detail.append(breadcrumb);
		} else if (node.en) detail.append(element("p", "english", node.en));
		detail.append(element("p", "summary", node.summary));
		detail.append(element("h3", "", "直觉 · 先抓住这个想法"), element("p", "body-copy", node.intuition));
		detail.append(element("div", "formula", node.formula));
		if (node.formulaNote) detail.append(element("div", "formula-note", node.formulaNote));
		detail.append(element("h3", "", "一个具体例子"), element("p", "body-copy", node.example));
		if (data.edges.some(([source, target]) => source === id || target === id)) detail.append(element("h3", "", "数学联系 · 为什么相连"));
		for (const [source, target, type, explanation] of data.edges) {
			if (source !== id && target !== id) continue;
			const other = nodes.get(source === id ? target : source);
			const relation = element("button", "relation");
			const label = type === "bridge" ? "相互联系" : source === id ? "继续学习" : "前置知识";
			const heading = element("strong", "", other.title);
			heading.append(element("span", "", "→"));
			relation.append(heading, element("small", "", `${label} · ${explanation}`));
			relation.addEventListener("click", () => {
				openNode(other.id, { locate: true });
				detail.querySelector("h2").focus({ preventScroll: true });
			});
			detail.append(relation);
		}
		if (node.course) {
			const course = courses.get(node.course);
			const chapter = course.chapters.find(chapter => chapter.nodes.includes(id));
			detail.append(element("h3", "", "同章索引 · 课程归属"), element("p", "formula-note", "以下为同一章节的知识点，归属相同不代表直接依赖。"));
			const siblings = element("div", "sibling-list");
			for (const sibling of chapter.nodes) {
				if (sibling === id) continue;
				const button = element("button", "domain-link", nodes.get(sibling).title);
				button.addEventListener("click", () => openNode(sibling, { locate: true }));
				siblings.append(button);
			}
			detail.append(siblings);
		} else if (node.courses) {
			detail.append(element("h3", "", "展开课程与全部知识点"));
			for (const courseId of node.courses) {
				const course = courses.get(courseId);
				const button = element("button", "course-jump", `${course.title} · ${course.chapters.flatMap(chapter => chapter.nodes).length} 个知识点 ↗`);
				button.addEventListener("click", () => { if (dialog.open) dialog.close(); filterDomain("all"); document.getElementById(`course-${courseId}`).scrollIntoView({ block: "start" }); });
				detail.append(button);
			}
		}
		detail.scrollTop = 0;
		if (updateUrl && location.hash !== `#${id}`) history.pushState(null, "", `#${id}`);
		if (mobile.matches) {
			dialog.append(detail);
			if (!dialog.open) dialog.showModal();
			dialog.scrollTop = 0;
			document.body.classList.add("detail-open");
		} else {
			if (locate) locateNode(id);
			const rect = detail.getBoundingClientRect();
			if (rect.top < 0 || rect.top > innerHeight * .6) window.scrollBy(0, rect.top - 12);
		}
	}

	function locateNode(id) {
		if (overviewIds.has(id)) document.querySelector("#overview-graph").open = true;
		buttons.get(id)?.scrollIntoView({ block: "center", inline: "nearest" });
	}

	function filterDomain(id) {
		domainFilter = id;
		hideTooltip();
		for (const section of catalog.querySelectorAll(".catalog-domain")) section.hidden = id !== "all" && section.dataset.domain !== id;
		for (const button of document.querySelectorAll("[data-filter]")) button.setAttribute("aria-pressed", String(button.dataset.filter === id));
		const visible = data.nodes.filter(node => node.course && (id === "all" || node.domain === id)).length;
		document.querySelector("#visible-count").textContent = `显示 ${visible} / ${conceptCount} 个知识点`;
	}

	function overview(updateUrl = true) {
		if (dialog.open) dialog.close();
		selected = null;
		filterDomain("all");
		hideTooltip();
		detail.style.removeProperty("--color");
		detail.replaceChildren(element("div", "detail-tag", "本科课程 / KNOWLEDGE ATLAS"), element("div", "overview-symbol", "∫"), element("h2", "", "把知识放回整体"), element("p", "english", `${data.courses.length} COURSES · ${conceptCount} CONCEPTS`), element("p", "summary", "从定义到定理，从课程到跨领域联系。这里铺开的是本科数学的课程骨架与具体知识点。"), element("h3", "", "怎样读这张地图"), element("p", "body-copy", "按颜色看领域，按卡片看课程，按分支看章节。全部词条默认展开；悬停看定义，点击看条件、公式和例子。详情中的数学联系可以继续跳转。"), element("h3", "", "从本科核心开始"));
		for (const id of ["linear", "calculus", "probability", "graphs"]) {
			const button = element("button", "domain-link", nodes.get(id).title);
			button.addEventListener("click", () => openNode(id));
			detail.append(button);
		}
		detail.append(element("h3", "", "知识之间有两种关系"), element("p", "body-copy", "章节分支表示“属于哪里”。数学联系则解释“为什么相连”：例如 Jacobian 的行列式连接线性代数中的体积与多元积分换元。两种关系分别显示。"), element("h3", "", "核心与选修"), element("p", "body-copy", "共同基础、本科核心、专业核心、专业选修与方向选修已标注。覆盖跨学校培养方案的主要知识范围，不把某校所有课程都视为人人必修。"));
		detail.scrollTop = 0;
		if (updateUrl && location.hash) history.pushState(null, "", location.pathname + location.search);
	}

	for (const domain of data.domains) {
		const territory = element("section", "territory");
		territory.style.setProperty("--color", domain.color);
		territory.style.left = `${domain.x}%`;
		territory.style.top = `${domain.y}%`;
		const heading = element("h3", "territory-heading", domain.name);
		heading.append(element("small", "", domain.en));
		territory.append(heading, element("p", "territory-question", domain.question));
		const children = data.nodes.filter(node => node.domain === domain.id && overviewIds.has(node.id));
		const rows = Math.ceil(children.length / 2);
		children.forEach((node, index) => {
			const left = index % 2 === 0 ? 6 : 52;
			const rowHeight = 54 / rows;
			const top = 37 + Math.floor(index / 2) * rowHeight;
			const height = rowHeight - 5;
			const button = element("button", "node", node.title);
			button.dataset.node = node.id;
			button.style.left = `${left}%`;
			button.style.top = `${top}%`;
			button.style.height = `${height}%`;
			button.addEventListener("mouseenter", () => showTooltip(node, button));
			button.addEventListener("mouseleave", hideTooltip);
			button.addEventListener("focus", () => showTooltip(node, button));
			button.addEventListener("blur", hideTooltip);
			button.addEventListener("click", () => openNode(node.id, { scroll: true }));
			buttons.set(node.id, button);
			positions.set(node.id, { x: (domain.x + 28 * (left + 21) / 100) * 10.8, y: (domain.y + 27 * (top + height / 2) / 100) * 8, halfWidth: 63.5, halfHeight: 27 * height * .04 });
			territory.append(button);
		});
		map.append(territory);
	}

	// 固定领域位置使多次阅读保持空间记忆；连线随数据生成。
	const ns = "http://www.w3.org/2000/svg";
	const defs = document.createElementNS(ns, "defs");
	const marker = document.createElementNS(ns, "marker");
	for (const [key, value] of Object.entries({ id: "arrow", viewBox: "0 0 10 10", refX: "9", refY: "5", markerWidth: "5", markerHeight: "5", orient: "auto-start-reverse" })) marker.setAttribute(key, value);
	const triangle = document.createElementNS(ns, "path");
	triangle.setAttribute("d", "M 0 0 L 10 5 L 0 10 z");
	triangle.setAttribute("fill", "#80937b");
	marker.append(triangle);
	defs.append(marker);
	svg.append(defs);
	for (const [source, target, type, , featured] of data.edges) {
		const a = positions.get(source);
		const b = positions.get(target);
		if (!a || !b) continue;
		const dx = b.x - a.x;
		const dy = b.y - a.y;
		const trim = point => Math.min(point.halfWidth / (Math.abs(dx) || 1), point.halfHeight / (Math.abs(dy) || 1));
		const start = { x: a.x + dx * trim(a), y: a.y + dy * trim(a) };
		const end = { x: b.x - dx * trim(b), y: b.y - dy * trim(b) };
		const path = document.createElementNS(ns, "path");
		const bend = nodes.get(source).domain === nodes.get(target).domain ? 0 : Math.min(55, Math.abs(dx) * .12);
		path.setAttribute("d", `M ${start.x} ${start.y} Q ${(start.x + end.x) / 2 + bend} ${(start.y + end.y) / 2 - bend} ${end.x} ${end.y}`);
		path.classList.add("edge");
		if (featured) path.classList.add("bridge");
		path.dataset.source = source;
		path.dataset.target = target;
		path.dataset.type = type;
		path.dataset.overview = String(Boolean(featured) || nodes.get(source).domain === nodes.get(target).domain);
		if (type === "prerequisite") path.setAttribute("marker-end", "url(#arrow)");
		svg.append(path);
	}

	// 课程树保持全部展开，领域筛选只改变可见范围，不改变数据或概念链接。
	for (const domain of data.domains) {
		const section = element("section", "catalog-domain");
		section.dataset.domain = domain.id;
		section.style.setProperty("--color", domain.color);
		const list = data.courses.filter(course => course.domain === domain.id);
		const count = list.reduce((sum, course) => sum + course.chapters.flatMap(chapter => chapter.nodes).length, 0);
		const heading = element("div", "domain-heading");
		heading.append(element("h3", "", domain.name), element("span", "", `${list.length} 门课程 · ${count} 个知识点`), element("small", "", domain.question));
		section.append(heading);
		const grid = element("div", "course-grid");
		for (const course of list) {
			const card = element("article", "course-card");
			card.id = `course-${course.id}`;
			card.dataset.course = course.id;
			const header = element("div", "course-header");
			header.append(element("span", "course-level", course.level), element("span", "", `${course.chapters.flatMap(chapter => chapter.nodes).length} 个知识点`));
			card.append(header, element("h4", "", course.title), element("p", "course-description", course.description));
			for (const chapter of course.chapters) {
				const branch = element("section", "chapter");
				branch.append(element("h5", "", chapter.title));
				const terms = element("div", "concept-list");
				for (const id of chapter.nodes) {
					const node = nodes.get(id);
					const button = element("button", "concept", node.title);
					button.dataset.node = id;
					button.addEventListener("mouseenter", () => showTooltip(node, button));
					button.addEventListener("mouseleave", hideTooltip);
					button.addEventListener("focus", () => showTooltip(node, button));
					button.addEventListener("blur", hideTooltip);
					button.addEventListener("click", () => openNode(id));
					buttons.set(id, button);
					terms.append(button);
				}
				branch.append(terms);
				card.append(branch);
			}
			grid.append(card);
		}
		section.append(grid);
		catalog.append(section);
	}
	for (const [id, title] of [["all", "全部领域"], ...data.domains.map(domain => [domain.id, domain.name])]) {
		const button = element("button", "", title);
		button.dataset.filter = id;
		button.addEventListener("click", () => filterDomain(id));
		document.querySelector("#domain-filters").append(button);
	}
	for (const [title, href] of data.sources) {
		const link = element("a", "", `${title} ↗`);
		link.href = href;
		document.querySelector("#curriculum-sources").append(link);
	}
	for (const [index, journey] of data.journeys.entries()) {
		const card = element("article", "journey");
		const title = element("h3", "journey-title");
		title.append(element("span", "", `0${index + 1}`), document.createTextNode(journey.title));
		const steps = element("div", "journey-steps");
		journey.nodes.forEach((id, step) => {
			if (step) steps.append(element("span", "", "→"));
			const button = element("button", "", nodes.get(id).title);
			button.addEventListener("click", () => openNode(id, { locate: true }));
			steps.append(button);
		});
		card.append(title, element("p", "", journey.description), steps);
		document.querySelector("#journey-list").append(card);
	}
	document.querySelector("#counts").textContent = `${data.courses.length} 门课程 / ${conceptCount} 个知识点`;
	document.querySelector("#coverage").textContent = `${chapterCount} 个章节 · ${data.edges.length} 条数学联系`;
	document.querySelector("#reset").addEventListener("click", () => overview());
	document.querySelector("#close-detail").addEventListener("click", () => dialog.close());
	dialog.addEventListener("close", () => {
		document.querySelector("#detail-host").append(detail);
		document.body.classList.remove("detail-open");
	});
	dialog.addEventListener("cancel", event => { event.preventDefault(); dialog.close(); });
	document.addEventListener("keydown", event => {
		if (dialog.open) return;
		if (event.key === "Escape") {
			if (preview) hideTooltip();
			else overview();
		}
	});
	window.addEventListener("resize", () => {
		hideTooltip();
		if (!mobile.matches && dialog.open) dialog.close();
	});
	window.addEventListener("scroll", () => {
		if (!tooltipOwner) return;
		const rect = tooltipOwner.getBoundingClientRect();
		if (rect.bottom < 0 || rect.top > innerHeight || rect.right < 0 || rect.left > innerWidth) hideTooltip();
		else showTooltip(nodes.get(tooltipOwner.dataset.node), tooltipOwner);
	}, true);
	function restoreUrl() {
		const id = location.hash.slice(1);
		if (nodes.has(id)) { filterDomain("all"); openNode(id, { updateUrl: false, locate: true }); }
		else overview(false);
	}
	window.addEventListener("hashchange", restoreUrl);
	window.addEventListener("popstate", restoreUrl);
	restoreUrl();
})();
