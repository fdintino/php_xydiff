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