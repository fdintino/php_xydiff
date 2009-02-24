<?php
$br = (php_sapi_name() == "cli")? "":"<br>";
$module = 'xydiff';
if(!extension_loaded('xydiff')) {
	dl($module . PHP_SHLIB_SUFFIX);
}

$functions = get_extension_funcs($module);
echo "Functions available in the test extension:$br\n";
foreach($functions as $func) {
    echo $func."$br\n";
}
echo "$br\n";

if (extension_loaded($module)) {
	$str = (class_exists('XyDiff')) ? 'XyDiff class exists' : 'Does not exist!';
	test_xydiff();
} else {
	$str = "Module $module is not compiled into PHP";
}
echo "$str\n";

function test_xydiff() {
	$dom1 = new XIDDOMDocument();
	$dom1->load("tests/example1.xml");
	$dom2 = new XIDDOMDocument();
	$dom2->load("tests/example2.xml");
	$xydiff = new XyDiff();
	$xydiff->setStartDocument($dom1);
	$xydiff->setEndDocument($dom2);

	$diffed = $xydiff->createDelta();
	echo $diffed->saveXML();
	echo $dom2->getXidMap();
	
	$xidtagged = $dom2->generateXidTaggedDocument();
	echo $xidtagged->saveXML();
}
?>
