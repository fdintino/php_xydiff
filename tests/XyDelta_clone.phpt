--TEST--
XyDelta - test object cloning
--SKIPIF--
<?php if (!extension_loaded("xydiff")) print "skip"; ?>
--FILE--
<?php
$path = dirname(__FILE__);
$file1 = realpath($path . '/' . 'example1.xml');
$file2 = realpath($path . '/' . 'example2.xml');
$dom1 = new XIDDOMDocument();
$dom1->load($file1);
$dom2 = new XIDDOMDocument();
$dom2->load($file2);
$xydiff = new XyDiff();
$delta = $xydiff->createDelta($dom1, $dom2);

$dom1a = new XIDDOMDocument();
$dom1a->load($file1);
$xydelta = new XyDelta();
$xydelta->setStartDocument($dom1a);
$xydeltaClone = clone $xydelta;
$result = $xydeltaClone->applyDelta($delta);

if ($result->saveXML() == $dom2->saveXML()) {
	echo "DeltaApply result matches original.";
} else {
	echo "DeltaApply result does not match original.";
}

?>
--EXPECT--
DeltaApply result matches original.
