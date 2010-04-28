--TEST--
XIDDOMDocument - test object cloning
--SKIPIF--
<?php if (!extension_loaded("xydiff")) print "skip"; ?>
--FILE--
<?php
function getStartXIDDOMDocument() {
	$path = dirname(__FILE__);
	$file1 = realpath($path . '/' . 'example1.xml');
	$dom1 = new XIDDOMDocument();
	$dom1->load($file1);
	return clone $dom1;
}

function getEndXIDDOMDocument() {
	$path = dirname(__FILE__);
	$file2 = realpath($path . '/' . 'example2.xml');
	$dom2 = new XIDDOMDocument();
	$dom2->load($file2);
	return clone $dom2;
}


$dom1 = getStartXIDDOMDocument();
$dom2 = getEndXIDDOMDocument();
$xydiff = new XyDiff();
$delta = $xydiff->createDelta($dom1, $dom2);
$dom1a = getStartXIDDOMDocument();
$xydelta = new XyDelta();
$xydelta->setStartDocument($dom1a);
$result = $xydelta->applyDelta($delta);

if ($result->saveXML() == $dom2->saveXML()) {
	echo "DeltaApply result matches original.";
} else {
	echo "DeltaApply result does not match original.";
}
?>
--EXPECT--
DeltaApply result matches original.
