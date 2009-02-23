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
	$dom1 = new XIDDOMDocument();
	$dom1->load("tests/example1.xml");
	$dom2 = new XIDDOMDocument();
	$dom2->load("tests/example2.xml");
	$xydiff = new XyDiff();
	$xydiff->loadXML($dom1);
	$diffed = $xydiff->diffXML($dom2);
//	echo $dom1->saveXML();
	echo $diffed->saveXML();
	// $xiddomdocument = new XIDDOMDocument();
	// $xiddomdocument->loadXML($dom1);
//	var_dump($xiddomdocument);
	
//	$xydiff->echoDelta("example1.xml", "example2.xml");
//	echo($xydiff->get_libxml_dom_string());
}
?>
