const installer = document.querySelector("#installer");
const status = document.querySelector("#install-status");
const versionLabel = document.querySelector("#firmware-version");
const manifestUrl = new URL("./manifest.json", import.meta.url);
const installerModule = "https://unpkg.com/esp-web-tools@10.5.2/dist/web/install-button.js?module";

async function loadInstaller() {
  status.hidden = false;
  if (location.protocol === "file:" || !window.isSecureContext) {
    status.textContent = "Open this installer over HTTPS or in the localhost preview.";
    return;
  }
  if (typeof navigator.serial?.requestPort !== "function") {
    status.textContent = "USB installation is unavailable in this browser. Allow serial ports in your site settings or use a desktop browser with Web Serial enabled.";
    return;
  }

  try {
    const response = await fetch(manifestUrl, { signal: AbortSignal.timeout(10000) });
    if (!response.ok) throw new Error("Firmware manifest unavailable");
    const manifest = await response.json();
    const part = manifest.builds?.find((build) => build.chipFamily === "ESP32-S3")?.parts?.[0];
    if (!manifest.version || !part?.path || part.offset !== 0) throw new Error("Invalid firmware manifest");
    versionLabel.textContent = `v${manifest.version}`;
  } catch {
    status.textContent = "The firmware manifest is unavailable. Reload the page, or check that the installer build was deployed in full.";
    return;
  }

  let timer;
  try {
    await Promise.race([
      import(installerModule),
      new Promise((_, reject) => {
        timer = setTimeout(() => reject(new Error("ESP Web Tools load timeout")), 12000);
      }),
    ]);
    if (!customElements.get("esp-web-install-button")) throw new Error("ESP Web Tools not registered");
    installer.hidden = false;
    status.hidden = true;
  } catch {
    status.textContent = "The web flasher could not load. Check your internet connection and reload, or download the firmware below.";
  } finally {
    clearTimeout(timer);
  }
}

loadInstaller();
