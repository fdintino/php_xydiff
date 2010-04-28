--TEST--
XyDelta::deltaApply() - basic test for XyDelta::deltaApply
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
$result = $xydelta->applyDelta($delta);

if ($result->saveXML() == $dom2->saveXML()) {
	echo "DeltaApply result matches original.";
} else {
	echo "DeltaApply result does not match original.";
}
/*
	you can add regression tests for your extension here

  the output of your test code has to be equal to the
  text in the --EXPECT-- section below for the tests
  to pass, differences between the output and the
  expected text are interpreted as failure

	see php5/README.TESTING for further information on
  writing regression tests
*/
?>
--EXPECT--
DeltaApply result matches original.
