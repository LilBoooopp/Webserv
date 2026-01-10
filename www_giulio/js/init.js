function init() {
  const c = [window.innerWidth / 2, window.innerHeight * 0.4];

  addDiv("WebServ", [c[0], c[1] - 150], 3, "white").tag = "home";
  addDarkModeButton();
  initBackground(true);

  const colSpacing = 200;
  const rowSpacing = 50;

  const menus = ["Navigation", "Requests", "CGI", "Post"];
  const totalWidth = (menus.length - 1) * colSpacing;

  const colX = [];
  for (let i = 0; i < menus.length; i++) {
    const x = c[0] - totalWidth / 2 + i * colSpacing;
    colX.push(x);
    addDiv(menus[i], [x, c[1] - 40], 1.5, "white").tag = "home";
  }

  const navX = colX[0];
  const reqX = colX[1];
  const cgiX = colX[2];
  const postX = colX[3];

  var yp = c[1] + 25;
  addButton("LOGIN", [navX, yp], () => showTag("login"), null, null, "GET /login.html HTTP/1.1", "home");
  addButton("/ressources", [navX, yp + rowSpacing], () => (window.location.href = "/ressources"), null, null, "GET /ressources HTTP/1.1", "home");

  var y = -1;
  addButton("SPAM", [reqX, yp + ++y * rowSpacing], loopRequest, null, null, "GET / HTTP/1.1 (x1000)", "home");
  addButton("not found", [reqX, yp + ++y * rowSpacing], () => (window.location.href = "/notfound"), null, null, "GET /notfound HTTP/1.1", "home");
  addButton("bad url", [reqX, yp + ++y * rowSpacing], () => (window.location.href = "awfin/qwq1||//#"), null, null, "GET awfin/qwq1||//# HTTP/1.1", "home");
  addButton("Unauthorized", [reqX, yp + ++y * rowSpacing], () => (window.location.href = "/ressources/secret/secret.txt"), null, null, "GET /ressources/secret/secret.txt HTTP/1.1", "home");
  addButton("Config", [reqX, yp + ++y * rowSpacing], () => parser.show(), null, null, "parser.show()", "home");

  const cgPaths = ["printArg.py", "infinite.py", "hugeResponse.py"];
  for (let i = 0; i < cgPaths.length; i++) {
    cgiButton(cgPaths[i], "cgi-bin/test/" + cgPaths[i], [cgiX, yp + rowSpacing * i], "Cool", announceResponse).tag = "home";
  }
  addInfiniteRequestButton([cgiX, yp + rowSpacing * cgPaths.length]).tag = "home";
  y = -1;
  addButton("GDRIVE", [postX, yp + ++y * rowSpacing], () => (window.location.href = "/cgi-bin/gdrive.php"), null, null, "GET /cgi-bin/drive.php HTTP/1.1", "home");
  addButton("1 Byte", [postX, yp + ++y * rowSpacing], () => postBytes(1), null, null, "POST /uploads/postBytes/1.txt", "home");
  addButton("1 MB", [postX, yp + ++y * rowSpacing], () => postBytes(1_000_000), null, null, "POST /uploads/postBytes/1000000.txt", "home");
  addButton("10 MB", [postX, yp + ++y * rowSpacing], () => postBytes(10_000_000), null, null, "POST /uploads/postBytes/10000000.txt", "home");
  addButton("1 GB", [postX, yp + ++y * rowSpacing], () => postBytes(999_999_999), null, null, "POST /uploads/postBytes/999999999.txt", "home");

  initLogin(c);
}

function initLogin(c) {
  addDiv("Login", [c[0], c[1] - 150], 3, "white", false, "login").tag = "login";
  const loginBtn = document.querySelector('button[name="action"][value="login"]');
  loginBtn.tag = "login";
  const registerBtn = document.querySelector('button[name="action"][value="register"]');
  registerBtn.tag = "login";
  const allLoginElements = document.querySelectorAll("#createForm, #createForm *");
  allLoginElements.forEach((el) => (el.tag = "login"));
  addInfo(loginBtn, "POST /cgi-bin/auth/sessionManagment/login.php HTTP/1.1\n - username: ...\n - password: ...");
  addInfo(registerBtn, "POST /cgi-bin/auth/sessionManagment/register.php HTTP/1.1\n - username: ...\n - password: ...");
  addButton("HOME", [c[0], window.innerHeight - 40], () => showTag("home")).tag = "login";
}
