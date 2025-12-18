<?php
session_start();

if (empty($_SESSION["user_id"])) {
	header("Location: /login.html");
	exit();
}
$user = $_SESSION["username"] ?? null;
$darkmode = $_SESSION["darkmode"] ?? null;
$name = $_SESSION["name"] ?? null;
$tel = $_SESSION["tel"] ?? null;
$email = $_SESSION["email"] ?? null;
$secret = $_SESSION["secret"] ?? null;
?>
<!DOCTYPE html>
<html>
  <head>
    <meta charset="UTF-8" />
    <meta name="viewport" content="width=device-width, initial-scale=1.0, maximum-scale=1.0, user-scalable=no" />
    <link rel="stylesheet" href="/css/style.css" />
  </head>

  <body>
    <div id="ui-root"></div>

    <script src="/js/div.js"></script>
    <script src="/js/buttons.js"></script>
    <script src="/js/auth.js"></script>
    <script src="/js/inputField.js"></script>

    <script>
			window.CURRENT_USER = <?php echo json_encode($user); ?>;
		window.DARKMODE = <?php echo json_encode($darkmode); ?>;
		window.PAGE_NAME = "about";
		var data = {
			"name": <?php echo json_encode($name); ?>,
			"tel": <?php echo json_encode($tel); ?>,
			"email": <?php echo json_encode($email); ?>,
			"secret": <?php echo json_encode($secret); ?>,
		}

		const c = [window.innerWidth / 2, window.innerHeight / 2];
		addTitle();
		const formFields = {};
		const formLabels = ["name", "email", "secret", "tel"];
		const w = 25;

		for (let i = 0; i < formLabels.length; i++) {
			const label = formLabels[i];
			const input = addInputField(label, [c[0], c[1] + w * i]);
			const div = addDiv(label, [c[0] - 150, c[1] + w * i - 10]);
			const value = data[label];
				if (value != null && value !== "")
				input.value = value;
			formFields[label] = input;
		}

		function f(){
			const data = new URLSearchParams();
			const headerData = {};
			for (const label of formLabels) {
				const input = formFields[label];
				data.append(label, input.value);
				headerData[label] = input.value;
				console.log(label, "=>", input.value);
			}
			fetch("/cgi-bin/auth/setters/setAbout.php", {
				method: "POST",
				headers: {
					"Content-Type": "application/x-www-form-urlencoded",
					"X-Name": headerData.name ?? "",
					"X-Tel": headerData.tel ?? "",
					"X-Email": headerData.email ?? "",
					"X-Secret": headerData.secret ?? "",
				},
				body: data.toString(),
			})
				.then((res) => res.text().then((t) => ({ res, t })))
				.then(({ res, t }) => {
				if (res.ok && t.trim() === "OK") announce("Saved");
					else announce(t || "Error saving");
				})
				.catch(() => announce("Can't save data"));
		}

		addButton("Send", [c[0] - 10, c[1] + w * (formLabels.length + 1) ], f, null, null, "POST /cgi-bin/auth/setters/setAbout.php\n - Content-Type: application/x-www-form-urlencoded\n - X-Name: ...\n - X-Tel: ...\n - X-Email: ...\n - X-Secret: ...");
		addScrollerProfileMenu();
		initBackground();
    </script>
  </body>
</html>
