// JTS GO website frontend logic
// Fetches data from the JTS GO JSON API and renders it.

function fetchJson(path, onOk, onErr) {
  fetch(path)
    .then(function (r) { return r.json(); })
    .then(onOk)
    .catch(function (e) { if (onErr) onErr(e); });
}

// ---- Home page stats ----
function loadHomeStats() {
  if (!document.getElementById("stats")) return;
  fetchJson("/api/projects", function (data) {
    var n = (data && data.projects) ? data.projects.length : 0;
    var el = document.getElementById("s-projects");
    if (el) el.textContent = n;
  });
  fetchJson("/api/skills", function (data) {
    var n = (data && data.skills) ? data.skills.length : 0;
    var el = document.getElementById("s-skills");
    if (el) el.textContent = n;
  });
}

// ---- Projects page ----
function loadProjects() {
  var grid = document.getElementById("projects");
  if (!grid) return;

  fetchJson("/api/projects", function (data) {
    var projects = (data && data.projects) ? data.projects : [];
    if (data && document.getElementById("projects-json")) {
      document.getElementById("projects-json").textContent = JSON.stringify(data, null, 2);
    }
    grid.innerHTML = "";
    projects.forEach(function (p) {
      var card = document.createElement("div");
      card.className = "card";
      var tags = (p.tags || []).map(function (t) {
        return '<span class="project-tag">' + t + '</span>';
      }).join("");
      card.innerHTML =
        '<div class="icon">' + (p.icon || "📦") + '</div>' +
        '<h3>' + p.name + '</h3>' +
        '<p>' + p.description + '</p>' +
        '<div>' + tags + '</div>';
      grid.appendChild(card);
    });
  });
}

// ---- About page ----
function loadAbout() {
  var el = document.getElementById("about-json");
  if (!el) return;
  fetchJson("/api/about", function (data) {
    el.textContent = JSON.stringify(data, null, 2);
  });
}

// ---- Contact page ----
function setupContact() {
  var form = document.getElementById("contact-form");
  if (!form) return;
  form.addEventListener("submit", function (ev) {
    ev.preventDefault();
    var name = document.getElementById("cf-name").value;
    var email = document.getElementById("cf-email").value;
    var message = document.getElementById("cf-message").value;
    var result = document.getElementById("cf-result");
    if (!name || !email || !message) {
      result.textContent = "Please fill in all fields.";
      result.style.color = "#ff6b6b";
      return;
    }
    result.textContent = "✓ Message received, " + name + "! (JTS GO backend would process this here.)";
    result.style.color = "#00ff88";
    form.reset();
  });
}

loadHomeStats();
loadProjects();
loadAbout();
setupContact();
