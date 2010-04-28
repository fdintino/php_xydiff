--TEST--
XyDelta - test reference counting
--SKIPIF--
<?php if (!extension_loaded("xydiff")) print "skip"; ?>
--FILE--
<?php
function createDelta() {
	$path = dirname(__FILE__);
	$file1 = realpath($path . '/' . 'example1.xml');
	$dom1a = new XIDDOMDocument();
	$dom1a->load($file1);
	$xydelta = new XyDelta();
	$xydelta->setStartDocument($dom1a);
	return $xydelta;
}
$path = dirname(__FILE__);
$file1 = realpath($path . '/' . 'example1.xml');
$file2 = realpath($path . '/' . 'example2.xml');
$dom1 = new XIDDOMDocument();
$dom1->load($file1);
$dom2 = new XIDDOMDocument();
$dom2->load($file2);
$xydiff = new XyDiff();
$delta = $xydiff->createDelta($dom1, $dom2);

$xydelta = createDelta();
$result = $xydelta->applyDelta($delta);

if ($result->saveXML() == $dom2->saveXML()) {
	echo "DeltaApply result matches original.";
} else {
	echo "DeltaApply result does not match original.";
}
?>
--EXPECT--
DeltaApply result matches original.
