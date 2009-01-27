<?php
$br = (php_sapi_name() == "cli")? "":"<br>";

if(!extension_loaded('xydiff')) {
	dl('xydiff.' . PHP_SHLIB_SUFFIX);
}
$module = 'xydiff';
$functions = get_extension_funcs($module);
echo "Functions available in the test extension:$br\n";
foreach($functions as $func) {
    echo $func."$br\n";
}
echo "$br\n";

if (extension_loaded($module)) {
	$str = (class_exists('XyDiff')) ? 'XyDiff class exists' : 'Does not exist!';
	test_xydiff();
	echo "XyDiff::public_property = $xydiff->public_property\n";
	
} else {
	$str = "Module $module is not compiled into PHP";
}
echo "$str\n";

function test_xydiff() {
	$dom1 = new DOMDocument();
	$dom1->load("example1.xml");
	$dom2 = new DOMDocument();
	$dom2->load("example2.xml");
	$xydiff = new XyDiff();
	$xydiff->loadXML($dom1);
	echo $xydiff->diffXML($dom2);
//	$xydiff->echoDelta("example1.xml", "example2.xml");
//	echo($xydiff->get_libxml_dom_string());
}
?>
