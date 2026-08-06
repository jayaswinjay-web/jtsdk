#!/usr/bin/env node

const fs = require("fs");
const https = require("https");
const path = require("path");
const { spawnSync } = require("child_process");

const OWNER = "jayaswinjay-web";
const REPO = "jtsdk";

const version = process.env.npm_package_version
  ? process.env.npm_package_version
  : require(path.join(__dirname, "package.json")).version;

const isWindows = process.platform === "win32";
const binaryName = isWindows ? "jts.exe" : "jts";

let subDir;
let asset;
if (process.platform === "win32" && process.arch === "x64") {
  subDir = "win32";
  asset = "jts-win32-x64.zip";
} else if (process.platform === "linux" && process.arch === "x64") {
  subDir = "linux";
  asset = "jts-linux-x64.tar.gz";
} else if (process.platform === "darwin" && process.arch === "arm64") {
  subDir = "darwin";
  asset = "jts-darwin-arm64.tar.gz";
} else {
  console.log(
    "[jts-go] No pre-built binary for " + process.platform + "/" + process.arch
  );
  console.log("[jts-go] Build from source: see https://github.com/jayaswinjay-web/jtsdk");
  process.exit(0);
}

const binDir = path.join(__dirname, "bin");
const outDir = path.join(binDir, subDir);
const binaryPath = path.join(outDir, binaryName);

function download(url, dest) {
  return new Promise((resolve, reject) => {
    const req = https.get(url, (res) => {
      if (res.statusCode >= 300 && res.statusCode < 400 && res.headers.location) {
        https.get(res.headers.location, (res2) => {
          if (res2.statusCode !== 200) {
            reject(new Error("download failed: HTTP " + res2.statusCode));
            return;
          }
          res2.pipe(fs.createWriteStream(dest).on("finish", resolve));
        }).on("error", reject);
        return;
      }
      if (res.statusCode !== 200) {
        reject(new Error("download failed: HTTP " + res.statusCode));
        return;
      }
      res.pipe(fs.createWriteStream(dest).on("finish", resolve));
    });
    req.on("error", reject);
  });
}

async function main() {
  if (fs.existsSync(binaryPath)) {
    console.log("[jts-go] JTS GO Development Kit v" + version + " installed successfully!");
    console.log("[jts-go] Run 'jts yourfile.jts' to get started.");
    return;
  }

  const url = "https://github.com/" + OWNER + "/" + REPO + "/releases/download/v" + version + "/" + asset;
  const tmp = path.join(binDir, ".download." + asset);

  console.log("[jts-go] Downloading " + asset + " ...");
  fs.mkdirSync(outDir, { recursive: true });

  try {
    await download(url, tmp);
  } catch (err) {
    console.error("[jts-go] Failed to download " + asset + ": " + err.message);
    console.error("[jts-go] Download manually from https://github.com/jayaswinjay-web/jtsdk/releases");
    process.exit(1);
  }

  const tar = spawnSync("tar", ["-xf", tmp, "-C", outDir], { stdio: "inherit" });
  fs.unlinkSync(tmp);

  if (tar.status !== 0 || !fs.existsSync(binaryPath)) {
    console.error("[jts-go] Failed to unpack " + asset);
    process.exit(1);
  }

  if (!isWindows) {
    fs.chmodSync(binaryPath, 0o755);
  }

  console.log("[jts-go] JTS GO Development Kit v" + version + " installed successfully!");
  console.log("[jts-go] Run 'jts yourfile.jts' to get started.");
  console.log("[jts-go] Docs: https://github.com/jayaswinjay-web/jtsdk#readme");
}

main();
