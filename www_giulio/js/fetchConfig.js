class confParser {
  constructor(pathToFile) {
    this.init(pathToFile).then(() => {
      this.log();
      this.initUI();
      this.show(false);
    });
  }

  show(shown = !this.shown) {
    if (shown) showTag(this.tag);
    else showTag("home");
    this.shown = shown;
  }

  initUI() {
    var c = [window.innerWidth / 2, window.innerHeight / 2];
    var sz = [window.innerWidth * 0.8, window.innerHeight * 0.6];
    var p = [c[0] - sz[0] / 2, window.innerHeight * 0.175];
    var tag = (this.tag = "parser");

    addDiv(this.filePath, [c[0], 100], 3, "white", null, true, tag);
    var btn = addButton("HOME", [c[0], window.innerHeight - 50], () => this.show());
    btn.tag = tag;
    var numServers = this.servers.length;
    var wSpot = 500;
    var hSpacing = 20;
    var x = c[0] - (numServers * wSpot) / 2;
    x += wSpot / 2;
    var colors = ["rgba(115, 174, 246, 1)", "rgba(119, 227, 248, 1)", "rgba(119, 248, 227, 1)"];
    for (let i = 0; i < numServers; i++) {
      var serv = this.servers[i];
      var color = colors[i % colors.length];
      var yp = p[1] + 50;
      var startY = yp;

      var y = 0;
      var dvn = addDiv(serv.name, [x, yp + hSpacing * ++y], 2, color, null, true, tag);
      if (serv.root && serv.index) {
        dvn.style.cursor = "pointer";
        dvn.addEventListener("mousedown", () => {
          const protocol = window.location.protocol;
          const port = serv.listen.split(":").pop();
          const path = serv.index.startsWith("/") ? serv.index : "/" + serv.index;
          const url = `${protocol}//localhost:${port}${path}`;
          window.location.href = url;
        });
      }
      y++;
      var xp = x - 100;
      addDiv(`listen`, [xp, yp + hSpacing * ++y], 1, color, null, false, tag);
      addDiv(`${serv.listen}`, [xp + 100, yp + 3 + hSpacing * y], 0.8, "lightgray", null, false, tag);

      if (serv.root) {
        addDiv(`root`, [xp, yp + hSpacing * ++y], 1, color, null, false, tag);
        addDiv(`${serv.root}`, [xp + 100, yp + 3 + hSpacing * y], 0.8, "lightgray", null, false, tag);
      }

      if (serv.index) {
        addDiv(`index`, [xp, yp + hSpacing * ++y], 1, color, null, false, tag);
        addDiv(`${serv.index}`, [xp + 100, yp + 3 + hSpacing * y], 0.8, "lightgray", null, false, tag);
      }

      if (serv.client_max_body_size) {
        addDiv(`client max`, [xp, yp + hSpacing * ++y], 1, color, null, false, tag);
        addDiv(`${serv.client_max_body_size}`, [xp + 100, yp + 3 + hSpacing * y], 0.8, "lightgray", null, false, tag);
      }

      if (Object.keys(serv.errorPages).length) {
        y += 2;
        var l = 0;
        var xx = 0;
        for (const [code, path] of Object.entries(serv.errorPages)) {
          var dv = addDiv(`${code}`, [xp + xx++ * 30, yp + hSpacing * y], 0.8, "lightgrey", null, false, tag);
          dv.style.cursor = "pointer";
          dv.addEventListener("mousedown", () => (window.location.href = path));
          if (++l % 7 === 0) {
            y++;
            xx = 0;
          }
        }
      }

      y++;
      for (const loc of serv.locations) {
        var locP = addDiv(`${loc.path}`, [xp, yp + hSpacing * ++y], 1, color, null, false, tag);
        locP.style.cursor = "pointer";
        locP.addEventListener("mousedown", () => (window.location.href = loc.path));
        if (loc.methods.length) addDiv(`\tmethods: ${loc.methods.join(", ")}`, [xp, yp + hSpacing * ++y], 0.8, "lightgray", null, false, tag);
        if (loc.root) addDiv(`\troot: ${loc.root}`, [xp, yp + hSpacing * ++y], 0.8, "lightgray", null, false, tag);
        if (loc.return) addDiv(`\treturn: ${loc.return}`, [xp, yp + hSpacing * ++y], 0.8, "lightgray", null, false, tag);
        if (loc.index) addDiv(`\treturn: ${loc.index}`, [xp, yp + hSpacing * ++y], 0.8, "lightgray", null, false, tag);
        if (loc.client_max_body_size) addDiv(`\tclient max\t${loc.client_max_body_size}b`, [xp, yp + hSpacing * ++y], 0.8, "lightgray", null, false, tag);
        if (Object.keys(loc.cgi).length) {
          for (const [ext, interp] of Object.entries(loc.cgi)) {
            addDiv(`\tcgi ${ext}\t${interp}`, [xp, yp + hSpacing * ++y], 0.8, "lightgray", null, false, tag);
          }
        }

        if (loc.autoindex !== null) addDiv(`\tautoindex: ${loc.autoindex}`, [xp, yp + hSpacing * ++y], 0.8, "lightgray", null, false, tag);
      }
      var curY = yp + hSpacing * ++y;
      const bw = 300;
      const bx = x - bw / 2 - 4;
      const by = startY - 4 - 20;
      const bh = curY - startY + 40;
      const bgr = writeBox(bw, bh, bx, by, "rgba(0, 0, 0, 0.1)", "rgba(255, 255, 255, .1)", 8);
      bgr.tag = tag;
      window.addEventListener("mousemove", (e) => {
        var mx = e.clientX,
          my = e.clientY;
        if (mx >= bx && mx < bx + bw && my >= by && my < by + bh) bgr.style.backgroundColor = "rgba(0, 0, 0, 0)";
        else bgr.style.backgroundColor = "rgba(0, 0, 0,.1)";
      });

      bgr.style.borderRadius = "20px";
      bgr.style.zIndex = -1;
      x += wSpot;
    }
  }
  parseConf(text) {
    // Remove comments
    text = text.replace(/#.*/g, "");

    // Tokenize - separate {, }, ; as individual tokens
    const tokens = [];
    const regex = /[{};]|"[^"]*"|'[^']*'|[^{}\s;]+/g;
    let match;
    while ((match = regex.exec(text)) !== null) {
      tokens.push(match[0]);
    }

    // Parse blocks recursively
    let pos = 0;

    function parseBlock() {
      const block = {};

      while (pos < tokens.length) {
        const token = tokens[pos++];

        if (token === "}") {
          return block;
        }

        if (token === "{") {
          continue;
        }
        const key = token;
        const values = [];
        let isBlock = false;

        while (pos < tokens.length) {
          const next = tokens[pos];

          if (next === ";") {
            pos++;
            break;
          }

          if (next === "{") {
            pos++;
            const nested = parseBlock();
            if (!block[key]) block[key] = [];
            block[key].push({ args: values, content: nested });
            isBlock = true;
            break;
          }

          values.push(tokens[pos++]);
        }

        // Simple directive (only add if not a block)
        if (!isBlock && values.length > 0) {
          if (!block[key]) block[key] = [];
          block[key].push(values);
        }
      }

      return block;
    }

    return parseBlock();
  }

  async init(filePath) {
    this.filePath = filePath;

    // Fetch and parse config file
    const response = await fetch(filePath);
    const confText = await response.text();
    const parsedConf = this.parseConf(confText);

    // Initialize servers array
    this.servers = [];

    if (parsedConf.server) {
      for (const serverBlock of parsedConf.server) {
        const content = serverBlock.content;

        // Build server config object
        const serverConfig = {
          name: content.server_name?.[0]?.[0] || "default",
          listen: content.listen?.[0]?.[0] || [],
          root: content.root?.[0]?.[0] || "/",
          index: content.index?.[0]?.[0] || null,
          client_max_body_size: content.client_max_body_size?.[0]?.[0] || null,
          errorPages: {},
          locations: [],
        };

        // Parse error pages: {code: path}
        if (content.error_page) {
          for (const ep of content.error_page) {
            serverConfig.errorPages[ep[0]] = ep[1];
          }
        }

        // Parse locations
        if (content.location) {
          for (const locBlock of content.location) {
            const locConfig = {
              path: locBlock.args[0],
              methods: locBlock.content.allowed_methods?.[0] || [],
              root: locBlock.content.root?.[0]?.[0],
              index: locBlock.content.index?.[0]?.[0] || null,
              autoindex: locBlock.content.autoindex?.[0]?.[0] || null,
              return: locBlock.content.return?.[0]?.join(" ") || null,
              client_max_body_size: locBlock.content.client_max_body_size?.[0]?.[0] || null,
              cgi: {},
              config: locBlock.content, // Full config for reference
            };
            // Convert cgi array to object {ext: interpreter}
            if (locBlock.content.cgi) {
              for (const cgiEntry of locBlock.content.cgi) {
                locConfig.cgi[cgiEntry[0]] = cgiEntry[1];
              }
            }
            serverConfig.locations.push(locConfig);
          }
        }

        this.servers.push(serverConfig);
      }
    }
  }

  log() {
    for (const s of this.servers) {
      console.log(`Server: ${s.name} => ${s.listen}`);
      console.log(`Root: ${s.root}`);

      console.log("Error Pages:");
      for (const [code, path] of Object.entries(s.errorPages)) {
        console.log(`  ${code} => ${path}`);
      }

      console.log("Locations:");
      for (const l of s.locations) {
        console.log(`  ${l.path}:`);
        console.log(`    Methods: ${l.methods.join(", ")}`);
        if (l.root) console.log(`    Root: ${l.root}`);
        console.log(`    Autoindex: ${l.autoindex}`);
      }
    }
  }
}
