const btns = document.getElementsByClassName("theme-toggle");
const darkToggles = document.getElementsByClassName("toggle-dark");
const lightToggles = document.getElementsByClassName("toggle-light");
const themeLink = document.querySelector("#theme-link");
let currentTheme = localStorage.getItem("theme");

function setDarkTheme(dark) {
  if (dark) {
    theme = "dark";
    otherTheme = "light";
  } else {
    theme = "light";
    otherTheme = "dark";
  }

  themeLink.href = themeLink.href.replace("site-" + otherTheme + ".css", "site-" + theme + ".css");

  for (let lt of lightToggles) {
    lt.hidden = !dark;
  }

  for (let dt of darkToggles) {
    dt.hidden = dark;
  }

  localStorage.setItem("theme", theme);
  currentTheme = theme;

  // Dispatch event so we know to reload Disqus (which will reset the colors).
  let event = new Event('colorSchemeChanged');
  document.dispatchEvent(event);
}

if (currentTheme === "light") {
  setDarkTheme(false);
} else if (window.matchMedia && window.matchMedia('(prefers-color-scheme: light)').matches) {
  setDarkTheme(false);
} else {
  currentTheme = "dark";
}

for (let btn of btns) {
  btn.addEventListener("click", function() {
    if (currentTheme === "light") {
      setDarkTheme(true);
    } else {
      setDarkTheme(false);
    }
  });
}
