<?php
$uploadsDir = realpath(__DIR__ . "/../ressources/uploads");
$currentDir = isset($_GET["dir"]) ? $_GET["dir"] : "";
$currentDir = ltrim($currentDir, "/");
$currentDir = preg_replace("~\.\.+~", "", $currentDir); // strip .. segments

$targetDir = realpath($uploadsDir . "/" . $currentDir);
if ($targetDir === false || strpos($targetDir, $uploadsDir) !== 0) {
	$targetDir = $uploadsDir;
	$currentDir = "";
}

$files = glob($targetDir . "/*");
$fileData = [];
foreach ($files as $file) {
	$pathinfo = pathinfo($file);
	$isDir = is_dir($file);
	$ext = $isDir ? "folder" : (isset($pathinfo["extension"]) ? $pathinfo["extension"] : "");
	$relativePath = trim(str_replace($uploadsDir, "", $file), "/");
	$fileData[] = [
		"name" => basename($file),
		"extension" => $ext,
		"size" => $isDir ? 0 : filesize($file),
		"path" => $file,
		"relativePath" => $relativePath,
		"isDir" => $isDir,
	];
}
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
    <script src="/js/gDrive.js"></script>

    <script>
      const files = <?php echo json_encode($fileData); ?>;
      const currentDir = <?php echo json_encode($currentDir); ?>;
      handleGDrive(files, currentDir);
	  	addDarkModeButton();
				initBackground(true);
    </script>
  </body>
</html>
