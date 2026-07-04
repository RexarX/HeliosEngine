// Edition switcher for multi-version GitHub Pages documentation.
// On GitHub Pages: loads /version_selector.html from the site root.
// Locally: loads version_selector.html next to the HTML output (via http.server).
(function () {
  function isPublishedSite(parts) {
    if (parts.length === 0) {
      return false;
    }
    var first = parts[0];
    if (first.endsWith(".html")) {
      return false;
    }
    // file:/// URLs on Windows may put a drive letter in the first segment.
    if (/^[a-zA-Z]:$/.test(first)) {
      return false;
    }
    return true;
  }

  function detectCurrentEdition(parts) {
    if (parts.length <= 1) {
      return "main";
    }
    if (parts[1] === "branches" && parts.length >= 3) {
      return "branches/" + parts[2];
    }
    if (/^\d+\.\d+/.test(parts[1]) && !parts[1].endsWith(".html")) {
      return parts[1];
    }
    return "main";
  }

  function showFallback() {
    var el = document.getElementById("projectnumber");
    if (!el) {
      return;
    }
    var fallback = el.getAttribute("data-fallback");
    if (fallback) {
      el.textContent = "\u00a0" + fallback;
    }
  }

  function wireSelector(parts, repoName) {
    var select = document.getElementById("versionSelector");
    if (!select) {
      showFallback();
      return;
    }

    if (select.disabled || select.options.length <= 1) {
      return;
    }

    select.addEventListener("change", function () {
      var selected = this.value;
      if (!repoName) {
        return;
      }
      var target =
        selected === "main"
          ? "/" + repoName + "/index.html"
          : "/" + repoName + "/" + selected + "/index.html";
      window.location.href = target;
    });

    if (repoName) {
      jQuery(select).val(detectCurrentEdition(parts));
    }
  }

  function loadSelector(selectorUrl, parts, repoName) {
    jQuery
      .get(selectorUrl, function (data) {
        jQuery("#projectnumber").html(data);
        wireSelector(parts, repoName);
      })
      .fail(showFallback);
  }

  function init() {
    if (typeof jQuery === "undefined") {
      showFallback();
      return;
    }

    var parts = window.location.pathname.split("/").filter(Boolean);
    if (window.location.protocol === "file:") {
      showFallback();
      return;
    }

    if (isPublishedSite(parts)) {
      var repoName = parts[0];
      loadSelector("/" + repoName + "/version_selector.html", parts, repoName);
      return;
    }

    loadSelector("version_selector.html", parts, null);
  }

  if (document.readyState === "loading") {
    document.addEventListener("DOMContentLoaded", init);
  } else {
    init();
  }
})();
