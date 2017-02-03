<?xml version="1.0"?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance" xmlns:dc="http://purl.org/dc/elements/1.1/" xmlns:oai_dc="http://www.openarchives.org/OAI/2.0/oai_dc/" xmlns:codeBook="ddi:codebook:2_5" version="1.0" xsi:schemaLocation="ddi:codebook:2_5 http://wwww.ddialliance.org/Specification/DDI-Codebook/2.5/XMLSchema/codebook.xsd" exclude-result-prefixes="codeBook">
  <xsl:output method="xml" version="1.0" encoding="UTF-8" indent="yes"/>
  <xsl:template match="/">
    <oai_dc:dc>
      <xsl:for-each select="//codeBook:citation/codeBook:titlStmt/codeBook:titl">
        <dc:title>
          <xsl:value-of select="."/>
        </dc:title>
      </xsl:for-each>
      <xsl:for-each select="//codeBook:citation/codeBook:titlStmt/codeBook:IDNo">
        <dc:identifier>
          <xsl:value-of select="."/>
        </dc:identifier>
      </xsl:for-each>
      <xsl:for-each select="//codeBook:citation/codeBook:rspStmt/codeBook:AuthEnty">
        <dc:creator>
          <xsl:value-of select="."/>
        </dc:creator>
      </xsl:for-each>
      <xsl:for-each select="//codeBook:citation/codeBook:rspStmt/codeBook:othId">
        <dc:contributor>
          <xsl:value-of select="."/>
        </dc:contributor>
      </xsl:for-each>
      <xsl:for-each select="//codeBook:citation/codeBook:prodStmt/codeBook:prodDate">
        <dc:date>
          <xsl:value-of select="."/>
        </dc:date>
      </xsl:for-each>
      <xsl:for-each select="//codeBook:stdyInfo/codeBook:subject/codeBook:keyword">
        <dc:subject>
          <xsl:value-of select="."/>
        </dc:subject>
      </xsl:for-each>
      <xsl:for-each select="//codeBook:stdyInfo/codeBook:abstract">
        <dc:description>
          <xsl:value-of select="."/>
        </dc:description>
      </xsl:for-each>
      <xsl:for-each select="//codeBook:stdyInfo/codeBook:sumDscr/codeBook:geogCover">
        <dc:coverage>
          <xsl:value-of select="."/>
        </dc:coverage>
      </xsl:for-each>
      <xsl:for-each select="//codeBook:stdyInfo/codeBook:sumDscr/codeBook:dataKind">
        <dc:type>
          <xsl:value-of select="."/>
        </dc:type>
      </xsl:for-each>
      <xsl:for-each select="//codeBook:method/codeBook:dataColl/codeBook:sources/codeBook:dataSrc">
        <dc:source>
          <xsl:value-of select="."/>
        </dc:source>
      </xsl:for-each>
      <xsl:for-each select="//codeBook:dataAccs/codeBook:useStmt/codeBook:restrctn">
        <dc:rights>
          <xsl:value-of select="."/>
        </dc:rights>
      </xsl:for-each>
      <xsl:for-each select="//codeBook:otherMat/codeBook:notes">
        <dc:isPartOf>
          <xsl:value-of select="."/>
        </dc:isPartOf>
      </xsl:for-each>
    </oai_dc:dc>
  </xsl:template>
</xsl:stylesheet>
