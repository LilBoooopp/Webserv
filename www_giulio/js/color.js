	const namedColors = {
		yellow: 'rgb(255,255,0)', blue: 'rgb(0,0,255)',
		brown: 'rgb(165,42,42)', red: 'rgba(255, 0, 0, 1)',
		green: 'rgba(0, 255, 0, 1)', black: "rgba(0, 0, 0, 1)",
		white: 'rgba(255, 255, 255, 1)'
	};

function randomizeColor(color, range = 10) {
  if (typeof color === 'string' && namedColors[color.toLowerCase()]) color = namedColors[color.toLowerCase()];
  const m = String(color).match(/^\s*rgba?\((\d{1,3}),\s*(\d{1,3}),\s*(\d{1,3})(?:,\s*[\d.]+)?\)\s*$/i);
	if (!m) {
		console.warn('rgb()/rgba() only');
		return `rgb(255, 255, 255)`;
	}
  let r = Math.max(0, Math.min(255, parseInt(m[1],10) + Math.floor(Math.random()*(2*range+1)) - range));
  let g = Math.max(0, Math.min(255, parseInt(m[2],10) + Math.floor(Math.random()*(2*range+1)) - range));
  let b = Math.max(0, Math.min(255, parseInt(m[3],10) + Math.floor(Math.random()*(2*range+1)) - range));
  return `rgb(${r}, ${g}, ${b})`;
}

function getRGB(color) {
  if (typeof color === 'string' && namedColors[color.toLowerCase()]) color = namedColors[color.toLowerCase()];
  const m = String(color).trim().match(/^\s*rgba?\((\d{1,3}),\s*(\d{1,3}),\s*(\d{1,3})(?:,\s*[\d.]+)?\)\s*$/i);
	if (!m) {
		console.warn(`Invalid color: ${color}`);
		return ([255,255,255]);
	}
  const rgb = [parseInt(m[1],10), parseInt(m[2],10), parseInt(m[3],10)];
	if (rgb.some(v => v < 0 || v > 255)) {
		console.warn(`RGB out of range: ${color}`);
		return ([255,255,255]);
	}
  return rgb;
}

function setAlpha(color, alpha) {
  const [r,g,b] = getRGB(color);
  return `rgba(${r},${g},${b},${alpha})`;
}

function addColor(originalColor, newColor, amount = 1) {
  const a = getRGB(originalColor), b = getRGB(newColor);
  const r = Math.round(Math.max(0, Math.min(255, a[0]*(1-amount)+b[0]*amount)));
  const g = Math.round(Math.max(0, Math.min(255, a[1]*(1-amount)+b[1]*amount)));
  const bl = Math.round(Math.max(0, Math.min(255, a[2]*(1-amount)+b[2]*amount)));
  return `rgb(${r}, ${g}, ${bl})`;
}

function getRainbowColor(FRAME = _frame, speed = 0.002) {
    const r = Math.floor(127 * Math.sin(speed * FRAME + 0) + 128);
    const g = Math.floor(127 * Math.sin(speed * FRAME + 2) + 128);
    const b = Math.floor(127 * Math.sin(speed * FRAME + 4) + 128);
    return `rgb(${r}, ${g}, ${b})`;
}

function getTimeColor() {
	let phase = 50;
	let t = _frame;
	let tr = t % 255;
	let tg = (t + phase) % 255;
	let tb = (t + phase * 2) % 255;
  	return `rgb(${tr}, ${tg}, ${tb})`;
}

function getMountainColor() {
	let phase = 50;
	let t = _frame;
	let tr = t % 255;
	let tg = (t + phase) % 255;
	let tb = (t + phase * 2) % 255;
  	return `rgb(${tr}, ${tg}, ${tb})`;
}

function getRandomColor() {
  	return `rgb(${r_range(0, 255)}, ${r_range(0, 255)}, ${r_range(0, 255)})`;
}

function recolorImage(imgPath, tintColor, callback) {
    const img = new Image();
    img.crossOrigin = "anonymous";
    img.src = imgPath;
    img.onload = () => {
        const canvas = document.createElement("canvas");
        canvas.width = img.width;
        canvas.height = img.height;
        const ctx = canvas.getContext("2d");

        ctx.drawImage(img, 0, 0);
        const imageData = ctx.getImageData(0, 0, canvas.width, canvas.height);
        const data = imageData.data;
        const rgb = tintColor.match(/\d+/g).map(Number);
        for (let i = 0; i < data.length; i += 4) {
            if (data[i + 3] > 0) { // skip fully transparent pixels
                data[i] = rgb[0];
                data[i + 1] = rgb[1];
                data[i + 2] = rgb[2];
            }
        }
        ctx.putImageData(imageData, 0, 0);
        callback(canvas.toDataURL("image/png"));
    };
    img.onerror = () => {
        console.error("Failed to load image:", imgPath);
        callback(null);
    };
}

function setBrightness(color, minLuminance = 150) {
	let r, g, b
    let returnAsString = false;
    if (typeof color === 'string') {
        const matches = color.match(/rgba?\((\d+),\s*(\d+),\s*(\d+)(?:,\s*([\d.]+))?\)/);
        if (!matches) return color;
        r = parseInt(matches[1]);
        g = parseInt(matches[2]);
        b = parseInt(matches[3]);
        returnAsString = true;
    } else if (Array.isArray(color)) {
        r = color[0];
        g = color[1];
        b = color[2];
    } else return color;
    let luminance = 0.299 * r + 0.587 * g + 0.114 * b;
    if (luminance < minLuminance && luminance > 0) {
        const scale = minLuminance / luminance;
        r = Math.min(255, Math.round(r * scale));
        g = Math.min(255, Math.round(g * scale));
        b = Math.min(255, Math.round(b * scale));
    }
    if (returnAsString) return `rgb(${r}, ${g}, ${b})`;
    return [r, g, b];
}