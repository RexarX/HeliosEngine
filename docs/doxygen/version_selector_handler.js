// Branch and version switchers — reads build-time data from edition_selector_data.js.
(function () {
  function isPublishedSite(parts) {
    if (parts.length === 0) {
      return false;
    }
    var first = parts[0];
    if (first.endsWith(".html")) {
      return false;
    }
    if (/^[a-zA-Z]:$/.test(first)) {
      return false;
    }
    return true;
  }

  function detectCurrentBranch(parts, select) {
    if (parts.length <= 1) {
      return "main";
    }
    if (parts[1] === "branches" && parts.length >= 3 && select) {
      var currentPath = parts.slice(1).join("/");
      var best = "";
      for (var i = 0; i < select.options.length; i++) {
        var value = select.options[i].value;
        if (
          value.indexOf("branches/") === 0 &&
          (currentPath === value || currentPath.indexOf(value + "/") === 0) &&
          value.length > best.length
        ) {
          best = value;
        }
      }
      if (best) {
        return best;
      }
    }
    return "main";
  }

  function detectCurrentVersion(parts) {
    if (
      parts.length >= 2 &&
      /^\d+\.\d+/.test(parts[1]) &&
      !parts[1].endsWith(".html")
    ) {
      return parts[1];
    }
    return window.__HELIOS_DEFAULT_VERSION__ || "";
  }

  function optionExists(select, value) {
    if (!select || !value) {
      return false;
    }
    for (var i = 0; i < select.options.length; i++) {
      if (select.options[i].value === value) {
        return true;
      }
    }
    return false;
  }

  function setSelectValue(select, value) {
    if (!select || !value) {
      return;
    }
    select.value = value;
    if (select.value !== value) {
      for (var i = 0; i < select.options.length; i++) {
        if (select.options[i].value === value) {
          select.selectedIndex = i;
          break;
        }
      }
    }
  }

  function wireBranchSelector(parts, repoName) {
    var select = document.getElementById("branchSelector");
    if (!select) {
      return;
    }

    if (repoName) {
      var fromPath = detectCurrentBranch(parts, select);
      if (optionExists(select, fromPath)) {
        setSelectValue(select, fromPath);
      }

      select.addEventListener("change", function () {
        var selected = this.value;
        if (!selected) {
          return;
        }
        var target =
          selected === "main"
            ? "/" + repoName + "/index.html"
            : "/" + repoName + "/" + selected + "/index.html";
        window.location.href = target;
      });
    }
  }

  function wireVersionSelector(parts, repoName) {
    var select = document.getElementById("versionSelector");
    if (!select) {
      return;
    }

    if (repoName) {
      var fromPath = detectCurrentVersion(parts);
      if (optionExists(select, fromPath)) {
        setSelectValue(select, fromPath);
      }

      select.addEventListener("change", function () {
        var selected = this.value;
        var current = detectCurrentVersion(parts);
        if (!selected || selected === current) {
          return;
        }
        window.location.href = "/" + repoName + "/" + selected + "/index.html";
      });
    }
  }

  function showFallbackBranch() {
    var container = document.getElementById("editionselector");
    if (!container || container.innerHTML.trim()) {
      return;
    }
    container.innerHTML =
      '<select id="branchSelector" title="Documentation branch">' +
      '<option value="">Branch unavailable</option></select>';
  }

  function showFallbackVersion() {
    var container = document.getElementById("versionselector");
    if (!container || container.innerHTML.trim()) {
      return;
    }
    var version = window.__HELIOS_DEFAULT_VERSION__ || "0.0.0";
    container.innerHTML =
      '<select id="versionSelector" title="Documentation version">' +
      '<option value="' +
      version +
      '">' +
      version +
      "</option></select>";
  }

  function init() {
    var parts = window.location.pathname.split("/").filter(Boolean);
    var repoName = isPublishedSite(parts) ? parts[0] : null;
    var branchContainer = document.getElementById("editionselector");
    var versionContainer = document.getElementById("versionselector");

    if (branchContainer && window.__HELIOS_BRANCH_SELECTOR_HTML__) {
      branchContainer.innerHTML = window.__HELIOS_BRANCH_SELECTOR_HTML__;
    } else {
      showFallbackBranch();
    }

    if (versionContainer && window.__HELIOS_VERSION_SELECTOR_HTML__) {
      versionContainer.innerHTML = window.__HELIOS_VERSION_SELECTOR_HTML__;
    } else {
      showFallbackVersion();
    }

    wireBranchSelector(parts, repoName);
    wireVersionSelector(parts, repoName);
  }

  if (document.readyState === "loading") {
    document.addEventListener("DOMContentLoaded", init);
  } else {
    init();
  }
})();
