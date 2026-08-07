#!/usr/bin/env node
// Cross-platform test runner for the JTS GO runtime.
// Usage: node scripts/test.js [path-to-jts-binary]
const { execFileSync } = require("child_process");
const fs = require("fs");
const path = require("path");

const root = path.resolve(__dirname, "..");
const isWindows = process.platform === "win32";
const binaryName = isWindows ? "jts.exe" : "jts";

let binary = process.argv[2];
if (!binary) {
  let subDir;
  if (process.platform === "win32") subDir = "win32";
  else if (process.platform === "linux") subDir = "linux";
  else if (process.platform === "darwin") subDir = "darwin";
  binary = path.join(root, "bin", subDir, binaryName);
}

if (!fs.existsSync(binary)) {
  console.error("test.js: binary not found: " + binary);
  process.exit(1);
}

function runJts(file) {
  try {
    execFileSync(binary, [file], { stdio: "pipe" });
    return 0;
  } catch (err) {
    return typeof err.status === "number" ? err.status : 1;
  }
}

const skipTests = new Set([
  "http_server.jts",
  "web_test.jts",
  "website.jts",
  "test_minimal.jts"
]);

const expected70 = new Set(["test_try_finally.jts"]);

let failed = 0;
let ran = 0;

function check(file, expected) {
  ran++;
  const status = runJts(file);
  if (status === expected) {
    console.log("PASS " + path.relative(root, file) + " (exit " + status + ")");
  } else {
    failed++;
    console.log("FAIL " + path.relative(root, file) + " expected " + expected + " got " + status);
  }
}

// Scrolls must all run clean.
const scrollsDir = path.join(root, "scrolls");
for (const f of fs.readdirSync(scrollsDir)) {
  if (f.endsWith(".jts")) check(path.join(scrollsDir, f), 0);
}

// Test suite.
const testsDir = path.join(root, "tests");
for (const f of fs.readdirSync(testsDir)) {
  if (!f.endsWith(".jts")) continue;
  if (skipTests.has(f)) continue;
  const expected = expected70.has(f) ? 70 : 0;
  check(path.join(testsDir, f), expected);
}

// Bring must resolve scrolls from a different working directory (walk-up
// from the binary's location, e.g. npm-installed packages).
try {
  const tmp = fs.mkdtempSync(path.join(require("os").tmpdir(), "jts-scroll-"));
  const probe = path.join(tmp, "scroll_probe.jts");
  fs.writeFileSync(probe, 'bring math\nsay("cwd-bring-ok")\n');
  ran++;
  try {
    execFileSync(binary, [probe], { cwd: tmp, stdio: "pipe" });
    console.log("PASS scrolls-from-other-cwd (exit 0)");
  } catch (err) {
    failed++;
    console.log("FAIL scrolls-from-other-cwd expected 0 got " + (typeof err.status === "number" ? err.status : 1));
  }
  fs.rmSync(tmp, { recursive: true, force: true });
} catch (err) {
  failed++;
  console.log("FAIL scrolls-from-other-cwd (setup error: " + err.message + ")");
}

console.log(ran + " tests, " + failed + " failed");
process.exit(failed ? 1 : 0);
