--TEST--
XIDDOMDocument - test for correct reference counting
--SKIPIF--
<?php if (!extension_loaded("xydiff")) print "skip"; ?>
--FILE--
<?php
function getStartXIDDOMDocument() {
	$path = dirname(__FILE__);
	$file1 = realpath($path . '/' . 'example1.xml');
	$dom1 = new XIDDOMDocument();
	$dom1->load($file1);
	return $dom1;
}

function getEndXIDDOMDocument() {
	$path = dirname(__FILE__);
	$file2 = realpath($path . '/' . 'example2.xml');
	$dom2 = new XIDDOMDocument();
	$dom2->load($file2);
	return $dom2;
}


$dom1 = getStartXIDDOMDocument();
$dom2 = getEndXIDDOMDocument();
$xydiff = new XyDiff();
$delta = $xydiff->createDelta($dom1, $dom2);
$dom1a = getStartXIDDOMDocument();
$xydelta = new XyDelta();
$xydelta->setStartDocument($dom1a);
$result = $xydelta->applyDelta($delta);

echo $result->saveXML();
?>
--EXPECT--
<?xml version="1.0" encoding="UTF-8" standalone="no"?>
<Groupe>

  <personne>
    <name>Cobena</name>
    <first>Gregory</first>
    <tel>5662</tel>
    <email>Gregory.Cobena@inria.fr</email>
  </personne>

  <personne away="yes">
    <name>Marian</name>
    <first>Amelie</first>
    <email>Amelie.Marian@inria.fr</email>
    <place>U. Columbia</place>
  </personne>

  <personne away="yes">
    <email>sjacqmin@sinorg.fr</email>
    <first>Sandrine</first>
    <name>Jacqmin</name>
  </personne>

  <personne away="no">
    <name>Mignet</name>
    <first>Laurent</first>
    <email>Laurent.Mignet@inria.fr</email>
    <tel>5722</tel>
    <tel>01.40.29.44.95</tel>
  </personne>

</Groupe>