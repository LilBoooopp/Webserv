function init() {
  const c = [window.innerWidth / 2, window.innerHeight * 0.4];
  const colSpacing = 200;
  const rowSpacing = 50;

  const menus = ["Navigation", "Requests", "CGI", "Post"];
  const totalWidth = (menus.length - 1) * colSpacing;

  const colX = [];
  for (let i = 0; i < menus.length; i++) {
    const x = c[0] - totalWidth / 2 + i * colSpacing;
    colX.push(x);
    const div = addDiv(menus[i], [x, c[1] - 40], 1.5, "white");
  }
  addDiv("WebServ", [c[0], c[1] - 150], 3, "white");
  addDarkModeButton();
  initBackground(true);
  const navX = colX[0];
  const reqX = colX[1];
  const cgiX = colX[2];
  const postX = colX[3];

  addButton("LOGIN", [navX, c[1] + 25], () => (window.location.href = "login.html"));
  addButton("/ressources", [navX, c[1] + 25 + rowSpacing], () => (window.location.href = "/ressources"));

  addButton("SPAM", [reqX, c[1] + 25], loopRequest);
  addButton("not found", [reqX, c[1] + 25 + rowSpacing], () => (window.location.href = "/notfound"));
  addButton("bad url", [reqX, c[1] + 25 + rowSpacing * 2], () => (window.location.href = "awfin/qwq1||//#"));

  const cgPaths = ["printArg.py", "infinite.py", "hugeResponse.py"];
  for (let i = 0; i < cgPaths.length; i++) {
    cgiButton(cgPaths[i], "cgi/test/" + cgPaths[i], [cgiX, c[1] + 25 + rowSpacing * i], "Cool", announceResponse);
  }
  addInfiniteRequestButton([cgiX, c[1] + 25 + rowSpacing * cgPaths.length]);
  var y = -1;
  addButton("GDRIVE", [postX, c[1] + 25 + ++y * rowSpacing], () => (window.location.href = "/cgi/gdrive.php"));
  addButton("1 Byte", [postX, c[1] + 25 + ++y * rowSpacing], () => postBytes(1));
  addButton("1 MB", [postX, c[1] + 25 + ++y * rowSpacing], () => postBytes(1_000_000));
  addButton("10 MB", [postX, c[1] + 25 + ++y * rowSpacing], () => postBytes(10_000_000));
  addButton("1 GB", [postX, c[1] + 25 + ++y * rowSpacing], () => postBytes(999_999_999));
}
