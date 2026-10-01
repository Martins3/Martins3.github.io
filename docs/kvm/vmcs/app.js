(() => {
	"use strict";
	const model = window.ATLAS;
	const fields = [...window.VMCS_FIELDS, ...window.VMCB_FIELDS];
	const byId = new Map(fields.map((f) => [f.id, f]));
	const groups = new Map(model.groups.map((g) => [g.id, g]));
	const areas = {
		control: "控制字段",
		guest: "Guest 状态",
		host: "Host 状态",
		exit: "退出信息",
	};
	const state = {
		arch: "vmcs",
		group: "all",
		area: "all",
		query: "",
		pinned: null,
		active: "vmcs-201a",
	};
	const $ = (s) => document.querySelector(s);
	const esc = (v) =>
		String(v).replace(
			/[&<>"']/g,
			(c) =>
				({
					"&": "&amp;",
					"<": "&lt;",
					">": "&gt;",
					'"': "&quot;",
					"'": "&#39;",
				})[c],
		);
	const style = (g) => `--accent:${g.color};--tint:${g.tint}`;
	let toastTimer;
	function toast(message) {
		$("#status").textContent = message;
		clearTimeout(toastTimer);
		toastTimer = setTimeout(() => {
			$("#status").textContent = "";
		}, 4500);
	}
	function relatedIds(id) {
		const result = new Set();
		for (const relation of model.relations)
			if (relation.includes(id))
				relation.forEach((v) => result.add(v));
		result.delete(id);
		return result;
	}
	function matchingFields() {
		const terms = state.query
			.toLowerCase()
			.trim()
			.split(/\s+/)
			.filter(Boolean);
		return fields.filter((f) => {
			if (state.arch !== "both" && f.arch !== state.arch)
				return false;
			if (state.group !== "all" && f.group !== state.group)
				return false;
			if (state.area !== "all" && f.area !== state.area)
				return false;
			const haystack = [
				f.name,
				f.official,
				f.summary,
				f.detail,
				f.encoding,
				f.high,
				f.section,
				groups.get(f.group).title,
			]
				.join(" ")
				.toLowerCase();
			return terms.every((t) => haystack.includes(t));
		});
	}
	function fieldButton(f) {
		return `<button class="field" data-field="${esc(f.id)}" aria-label="${esc(f.name + "，" + f.summary)}" title="点击固定；再次点击解除"><span class="field-label">${esc(f.name)}</span><span class="field-meta">${state.arch === "both" ? `<span class="arch-tag">${f.arch.toUpperCase()}</span>` : ""}<span class="short">${esc(f.summary)}</span><code>${f.encoding}</code></span></button>`;
	}
	function renderMap() {
		document.querySelectorAll("[data-arch]").forEach((b) =>
			b.setAttribute(
				"aria-pressed",
				String(b.dataset.arch === state.arch),
			),
		);
		$("#groups").innerHTML = [
			{ id: "all", title: "功能全景", color: "#5a6f4e" },
			...model.groups,
		]
			.map((g) => {
				const count = fields.filter(
					(f) =>
						(state.arch === "both" ||
							f.arch ===
								state.arch) &&
						(g.id === "all" ||
							f.group === g.id),
				).length;
				return `<button class="group-nav" data-group="${g.id}" aria-pressed="${state.group === g.id}" style="--accent:${g.color}"><span class="dot"></span>${g.title}<span class="nav-count">${count}</span></button>`;
			})
			.join("");
		document.querySelectorAll("[data-area]").forEach((button) => {
			button.setAttribute(
				"aria-pressed",
				String(button.dataset.area === state.area),
			);
		});
		const g = groups.get(state.group);
		$("#map-title").textContent = g ? g.title : "功能全景";
		$("#map-eyebrow").textContent =
			state.arch === "both"
				? "SAME PURPOSE, DIFFERENT ARCHITECTURES"
				: "THE BIG PICTURE";
		$("#architecture-note").textContent = g
			? g.detail
			: state.arch === "vmcs"
				? "VMCS 通过 VMREAD / VMWRITE 访问。卡片上的数字是字段编码，不是字节偏移。"
				: state.arch === "vmcb"
					? "VMCB 是固定布局的内存结构。数字是相对 VMCB 起点的字节偏移，Guest 保存区始于 0x400。"
					: "按功能并列观察 Intel 与 AMD。VMCS 显示编码，VMCB 显示偏移；同色表示相关用途，并非位级等价。";
		const matches = matchingFields();
		$("#field-map").innerHTML =
			model.groups
				.map((group, index) => {
					const groupFields = matches.filter(
						(field) =>
							field.group ===
							group.id,
					);
					if (!groupFields.length) return "";
					const sections =
						state.arch === "both"
							? ["vmcs", "vmcb"]
									.map(
										(
											arch,
										) => {
											const subset =
												groupFields.filter(
													(
														field,
													) =>
														field.arch ===
														arch,
												);
											return subset.length
												? '<div class="comparison-label">' +
														(arch ===
														"vmcs"
															? "INTEL VMCS · ENCODING"
															: "AMD VMCB · OFFSET") +
														"</div>" +
														subset
															.map(
																fieldButton,
															)
															.join(
																"",
															)
												: "";
										},
									)
									.join(
										"",
									)
							: groupFields
									.map(
										fieldButton,
									)
									.join(
										"",
									);
					return (
						'<section class="field-group" style="' +
						style(group) +
						'"><div class="group-heading"><div class="group-heading-top"><span class="group-number">0' +
						(index + 1) +
						"</span><h3>" +
						group.title +
						'</h3><span class="total">' +
						groupFields.length +
						"</span></div><p>" +
						group.question +
						'</p></div><div class="field-list">' +
						sections +
						"</div></section>"
					);
				})
				.join("") ||
			'<div class="empty"><strong>没有找到匹配字段</strong><p>试试 RIP、EPT、0x201a，或清除当前筛选。</p><button data-reset>重置筛选</button></div>';
		$("#count").textContent = matches.length + " 字段";
		highlight();
	}
	function highlight() {
		const related = relatedIds(state.active);
		document.querySelectorAll(".field").forEach((button) => {
			const active = button.dataset.field === state.active;
			button.classList.toggle("preview", active);
			button.classList.toggle(
				"selected",
				button.dataset.field === state.pinned,
			);
			button.setAttribute(
				"aria-pressed",
				String(button.dataset.field === state.pinned),
			);
			button.classList.toggle(
				"related",
				!active && related.has(button.dataset.field),
			);
			if (active) button.setAttribute("aria-current", "true");
			else button.removeAttribute("aria-current");
		});
	}
	function renderDetail() {
		const f = byId.get(state.active);
		if (!f) return;
		const g = groups.get(f.group),
			intel = f.arch === "vmcs";
		const definition = intel
			? "arch/x86/include/asm/vmx.h"
			: "arch/x86/include/asm/svm.h";
		const [kvmPath, kvmSymbol] = g[f.arch],
			related = [...relatedIds(f.id)]
				.map((id) => byId.get(id))
				.filter(Boolean);
		if ($("#detail").dataset.displayedId !== f.id)
			$("#inspector").scrollTop = 0;
		$("#detail").dataset.displayedId = f.id;
		$("#detail").style.cssText = style(g);
		$("#detail").innerHTML =
			`<div class="inspector-header"><span>${state.pinned ? "已固定 · 再点字段或 Esc 解除" : "字段说明"}</span><button class="back-to-map" id="back-to-map">返回字段地图 ↑</button></div><div class="detail-content">
      <div class="detail-kicker"><span class="dot"></span>${f.arch.toUpperCase()} / ${g.title}</div><h3 class="detail-title">${esc(f.name)}</h3><p class="official-name">${esc(f.official)}</p>
      <div class="badges"><span class="badge">${intel ? "encoding" : "offset"} ${f.encoding}</span><span class="badge">${esc(f.width)}</span><span class="badge">${areas[f.area]}</span>${f.high ? `<span class="badge">high ${f.high}</span>` : ""}</div>
      <div class="detail-summary">${esc(f.summary)}</div><section class="detail-section"><h4>它做什么</h4><p>${esc(f.detail)}</p></section>
      ${f.bits.length ? `<section class="detail-section"><h4>关键值 / 位段 · 节选</h4><table class="bits"><caption class="sr-only">${esc(f.name)} 关键位段</caption><tbody>${f.bits.map(([b, d]) => `<tr><td>${esc(b)}</td><td>${esc(d)}</td></tr>`).join("")}</tbody></table><p class="bit-foot">未列出的位请查手册；保留位与能力条件仍须满足。</p></section>` : ""}
      ${related.length ? `<section class="detail-section"><h4>一起理解 · 相关字段</h4><p class="relation-hint">点击跳转；跨架构关联表示功能相近。</p><div class="relations">${related.map((r) => `<button data-related="${esc(r.id)}" title="${esc(r.summary)}">${r.arch === f.arch ? "" : r.arch.toUpperCase() + " · "}${esc(r.name)}</button>`).join("")}</div></section>` : ""}
      <section class="detail-section"><h4>回到手册与源码</h4><p>${esc(f.section)}</p>${intel ? `<p class="source-note">${f.linux ? "本地 Linux 宏：" : "SDM 扩展字段；本地 enum vmcs_field 尚无对应宏。"}${f.linux ? `<code>${esc(f.name)}</code>` : ""}</p>` : '<p class="source-note">偏移从 VMCB 起点计算；以本地结构为准。</p>'}<code class="source-path">${definition}</code><p class="source-symbol">${intel ? "enum vmcs_field" : f.area === "guest" ? "struct vmcb_save_area / struct vmcb_seg" : "struct vmcb_control_area"}</p><p class="source-note">功能组阅读入口（并非每个字段的使用点）：</p><code class="source-path">${kvmPath}</code><code class="source-symbol">${esc(kvmSymbol)}</code></section>
<div class="detail-footer"><button id="copy-link">复制字段链接 ↗</button></div></div>`;
	}
	function setHash(id) {
		try {
			history.replaceState(
				null,
				"",
				"#" + encodeURIComponent(id),
			);
		} catch {
			/* file:// may disallow history changes */
		}
	}
	function activate(id, navigate = false) {
		if (!byId.has(id)) return;
		state.active = id;
		if (navigate) {
			const f = byId.get(id);
			if (state.arch !== f.arch && state.arch !== "both")
				state.arch = "both";
			state.group = "all";
			state.area = "all";
			state.query = "";
			$("#search").value = "";
			renderMap();
			document.querySelector(
				`[data-field="${CSS.escape(id)}"]`,
			)?.scrollIntoView({
				block: "nearest",
				behavior: "instant",
			});
		}
		renderDetail();
		highlight();
		if (navigate) {
			setHash(id);
			if (matchMedia("(max-width: 680px)").matches)
				$("#inspector").scrollIntoView({
					block: "start",
					behavior: "instant",
				});
		}
	}
	function reset() {
		Object.assign(state, {
			group: "all",
			area: "all",
			query: "",
			pinned: null,
		});
		$("#search").value = "";
		state.active =
			state.arch === "vmcb"
				? "vmcb-control.nested_cr3"
				: "vmcs-201a";
		renderMap();
		renderDetail();
		try {
			history.replaceState(
				null,
				"",
				location.pathname + location.search,
			);
		} catch {
			/* local files */
		}
	}
	$("#search").addEventListener("input", (e) => {
		state.query = e.target.value;
		renderMap();
	});
	$("#field-map").addEventListener("pointermove", (e) => {
		if (e.pointerType !== "mouse" || state.pinned) return;
		const f = e.target.closest("[data-field]");
		if (f && f.dataset.field !== state.active)
			activate(f.dataset.field);
	});
	$("#field-map").addEventListener("focusin", (e) => {
		const f = e.target.closest("[data-field]");
		if (f && !state.pinned) activate(f.dataset.field);
	});
	document.addEventListener("click", async (e) => {
		const b = e.target.closest("button");
		if (!b) return;
		if (b.dataset.area) {
			state.area = b.dataset.area;
			renderMap();
			return;
		}
		if (b.dataset.arch) {
			state.arch = b.dataset.arch;
			state.pinned = null;
			if (
				state.arch !== "both" &&
				byId.get(state.active).arch !== state.arch
			)
				state.active =
					state.arch === "vmcb"
						? "vmcb-control.nested_cr3"
						: "vmcs-201a";
			renderMap();
			renderDetail();
			return;
		}
		if (b.dataset.group) {
			state.group = b.dataset.group;
			renderMap();
			return;
		}
		if (b.dataset.field) {
			state.pinned =
				state.pinned === b.dataset.field
					? null
					: b.dataset.field;
			activate(b.dataset.field);
			setHash(b.dataset.field);
			if (matchMedia("(max-width: 680px)").matches)
				$("#inspector").scrollIntoView({
					block: "start",
					behavior: "instant",
				});
			return;
		}
		if (b.dataset.related) {
			state.pinned = b.dataset.related;
			activate(b.dataset.related, true);
			return;
		}
		if (b.hasAttribute("data-reset") || b.id === "reset") {
			reset();
			return;
		}
		if (b.id === "back-to-map") {
			const field = document.querySelector(
				'[data-field="' +
					CSS.escape(state.active) +
					'"]',
			);
			(field || $("#map")).scrollIntoView({
				block: "center",
				behavior: "instant",
			});
			if (field) field.focus({ preventScroll: true });
			return;
		}
		if (b.id === "copy-link") {
			const url = new URL(location.href);
			url.hash = state.active;
			try {
				await navigator.clipboard.writeText(url.href);
				toast("字段链接已复制");
			} catch {
				setHash(state.active);
				toast("请从地址栏复制当前字段链接");
			}
		}
	});
	document.addEventListener("keydown", (e) => {
		const editing = e.target.matches(
			"input,textarea,select,[contenteditable]",
		);
		if (e.key === "/" && !editing) {
			e.preventDefault();
			$("#search").focus();
		}
	});
	document.addEventListener("keydown", (e) => {
		if (e.key === "Escape" && state.pinned) {
			state.pinned = null;
			renderDetail();
			highlight();
		}
	});
	function openHash() {
		let id;
		try {
			id = decodeURIComponent(location.hash.slice(1));
		} catch {
			return;
		}
		if (byId.has(id)) {
			state.arch = byId.get(id).arch;
			state.pinned = null;
			activate(id, true);
		}
	}
	window.addEventListener("hashchange", openHash);
	renderMap();
	renderDetail();
	openHash();
})();
