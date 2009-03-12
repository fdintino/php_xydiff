<?php
$br = (php_sapi_name() == "cli")? "":"<br>";
$module = 'xydiff';
if(!extension_loaded('xydiff')) {
	dl($module . "." . PHP_SHLIB_SUFFIX);
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
	$delta = $xydiff->createDelta($dom1, $dom2);
	
	$result = test_xydelta("tests/example1.xml", $delta);
	echo '$xydelta->applyDelta($diffed)',"\n";
	echo $result->saveXML();

	// $xydelta = new XyDelta();
	// $xydelta->setStartDocument($dom1);
	// $delta = $xydelta->applyDelta($diffed);
	// echo '$xydelta->applyDelta($diffed)',"\n";
	// echo $delta->saveXML();
	
	echo $delta->saveXML();
	echo "dom2->getXidMap() = ", $dom1->getXidMap(), "\n";
	$xidmap = $dom2->getXidMap();
	echo "dom2->getXidMap() = $xidmap\n";
	$xidtagged = $dom2->generateXidTaggedDocument();
	echo "\n", '===== $xidtagged->saveXML() =====', "\n";
	echo $xidtagged->saveXML();
	test_xydiff_xidmap($xidmap);
	if ($result->saveXML() == $dom2->saveXML()) {
		echo "DeltaApply result matches original!\n";
	} else {
		echo "DeltaApply result does not match original.\n";
	}
}

function test_xydelta($file1, $delta) {
	$dom1 = new XIDDOMDocument();
	$dom1->load($file1);
	$xydelta = new XyDelta();
	$xydelta->setStartDocument($dom1);
	$result = $xydelta->applyDelta($delta);
	return $result;
}

function test_xydiff_xidmap($xidmap) {
	$dom2 = new XIDDOMDocument();
	$dom3 = new XIDDOMDocument();
	$dom2->load("tests/example2.xml");
	$dom2->setXidMap($xidmap);
	$dom3->load("tests/example3.xml");
	$xydiff = new XyDiff();
	$diffed = $xydiff->createDelta($dom2, $dom3);
	echo $diffed->saveXML();
	echo "dom2->getXidMap() = ", $dom2->getXidMap(), "\n";
	echo "dom3->getXidMap() = ", $dom3->getXidMap(), "\n";
}
?>
