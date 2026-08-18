(() => {
  "use strict";

  const book = document.querySelector("#book");
  const pages = Array.from(document.querySelectorAll(".page"));
  const restartButton = document.querySelector("[data-action='restart']");

  if (!book || pages.length === 0) {
    return;
  }

  const initialPage = Number.parseInt(window.location.hash.replace("#page-", ""), 10);
  let activeIndex = Number.isInteger(initialPage)
    ? Math.max(0, Math.min(initialPage - 1, pages.length - 1))
    : 0;
  let pointerStartX = null;
  let pointerStartY = null;
  let didSwipe = false;

  function render() {
    pages.forEach((page, index) => {
      page.classList.toggle("is-active", index === activeIndex);
      page.classList.toggle("is-before", index < activeIndex);
      page.classList.toggle("is-after", index > activeIndex);
      page.setAttribute("aria-hidden", index === activeIndex ? "false" : "true");
      page.style.zIndex = String(pages.length - Math.abs(index - activeIndex));
    });

    const title = pages[activeIndex].dataset.pageTitle;
    if (title) {
      book.setAttribute("aria-label", `第 ${activeIndex + 1} 页：${title}`);
    }
    window.history.replaceState(null, "", `#page-${activeIndex + 1}`);
  }

  function turnTo(index) {
    const nextIndex = Math.max(0, Math.min(index, pages.length - 1));
    if (nextIndex === activeIndex) {
      return;
    }
    activeIndex = nextIndex;
    render();
  }

  function turnNext() {
    turnTo(activeIndex + 1);
  }

  function turnPrevious() {
    turnTo(activeIndex - 1);
  }

  restartButton?.addEventListener("click", () => turnTo(0));

  book.addEventListener("click", (event) => {
    if (didSwipe) {
      didSwipe = false;
      return;
    }
    if (event.target.closest("button, a")) {
      return;
    }
    const bounds = book.getBoundingClientRect();
    if (event.clientX < bounds.left + bounds.width * 0.42) {
      turnPrevious();
    } else {
      turnNext();
    }
  });

  book.addEventListener("pointerdown", (event) => {
    pointerStartX = event.clientX;
    pointerStartY = event.clientY;
  });

  book.addEventListener("pointerup", (event) => {
    if (pointerStartX === null || pointerStartY === null) {
      return;
    }

    const deltaX = event.clientX - pointerStartX;
    const deltaY = event.clientY - pointerStartY;
    pointerStartX = null;
    pointerStartY = null;

    if (Math.abs(deltaX) < 46 || Math.abs(deltaX) < Math.abs(deltaY)) {
      return;
    }
    didSwipe = true;
    if (deltaX < 0) {
      turnNext();
    } else {
      turnPrevious();
    }
  });

  book.addEventListener("pointercancel", () => {
    pointerStartX = null;
    pointerStartY = null;
  });

  window.addEventListener("keydown", (event) => {
    if (event.key === "ArrowRight" || event.key === "PageDown") {
      event.preventDefault();
      turnNext();
    } else if (event.key === "ArrowLeft" || event.key === "PageUp") {
      event.preventDefault();
      turnPrevious();
    } else if (event.key === "Home") {
      event.preventDefault();
      turnTo(0);
    } else if (event.key === "End") {
      event.preventDefault();
      turnTo(pages.length - 1);
    }
  });

  render();
  window.requestAnimationFrame(() => {
    window.requestAnimationFrame(() => document.body.classList.add("is-ready"));
  });
})();
