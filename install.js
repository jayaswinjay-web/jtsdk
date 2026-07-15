#!/usr/bin/env node

const fs = require("fs");
const path = require("path");

const platform = process.platform;
const arch = process.arch;
const binDir = path.join(__dirname, "bin");

let subDir;
if (platform === "win32") {
  subDir = "win32";
} else if (platform === "linux") {
  subDir = "linux";
} else if (platform === "darwin") {
  subDir = "darwin";
} else {
  console.log("[jts-go] Skipping install: unsupported platform " + platform);
  process.exit(0);
}

const isWindows = platform === "win32";
const binaryName = isWindows ? "jts.exe" : "jts";
const binaryPath = path.join(binDir, subDir, binaryName);

if (fs.existsSync(binaryPath)) {
  console.log("[jts-go] JTS GO Development Kit v1.0 installed successfully!");
  console.log("[jts-go] Run 'jts yourfile.jts' to get started.");
  console.log("[jts-go] Docs: https://github.com/jayaswinjay-web/jtsdk#readme");
} else {
  console.log("[jts-go] Warning: Binary not found for " + platform + "/" + arch);
  console.log("[jts-go] Download manually from: https://github.com/jayaswinjay-web/jtsdk/releases");
}

if (!isWindows) {
  try {
    fs.chmodSync(binaryPath, 0o755);
  } catch (e) {
    // ignore
  }
}
