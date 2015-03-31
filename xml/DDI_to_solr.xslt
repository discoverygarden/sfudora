<?xml version="1.0" encoding="UTF-8"?>
<xsl:stylesheet version="1.0"
  xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
  xmlns:xlink="http://www.w3.org/1999/xlink"
  xmlns:foxml="info:fedora/fedora-system:def/foxml#"
  xmlns:codeBook="ddi:codebook:2_5">

  <xsl:template match="foxml:datastream[@ID='DDI']/foxml:datastreamVersion[last()]">
    <xsl:param name="content"/>
    <xsl:param name="prefix">ddi.</xsl:param>
    <xsl:param name="suffix"></xsl:param>

    <xsl:apply-templates select="$content/codeBook">
      <xsl:with-param name="prefix" select="$prefix"/>
      <xsl:with-param name="suffix" select="$suffix"/>
    </xsl:apply-templates>
  </xsl:template>

  <xsl:template match="codeBook">
    <xsl:param name="prefix">ddi.</xsl:param>
    <xsl:param name="suffix"></xsl:param>

<!-- SFUdora DDI subset -->

  <xsl:for-each select="//codeBook:citation/codeBook:titlStmt/codeBook:titl[normalize-space(text())]">
    <field>
      <xsl:attribute name="name">
        <xsl:value-of select="concat($prefix, local-name(), $suffix)"/>
      </xsl:attribute>
      <xsl:value-of select="text()"/>
    </field>
  </xsl:for-each>

  <xsl:for-each select="//codeBook:citation/codeBook:titlStmt/codeBook:IDNo[normalize-space(text())]">
    <field>
      <xsl:attribute name="name">
        <xsl:value-of select="concat($prefix, local-name(), $suffix)"/>
      </xsl:attribute>
      <xsl:value-of select="text()"/>
    </field>
  </xsl:for-each>

  <xsl:for-each select="//codeBook:citation/codeBook:rspStmt/codeBook:AuthEnty[normalize-space(text())]">
    <field>
      <xsl:attribute name="name">
        <xsl:value-of select="concat($prefix, local-name(), $suffix)"/>
      </xsl:attribute>
      <xsl:value-of select="text()"/>
    </field>
  </xsl:for-each>

  <xsl:for-each select="//codeBook:citation/codeBook:rspStmt/codeBook:othId[normalize-space(text())]">
    <field>
      <xsl:attribute name="name">
        <xsl:value-of select="concat($prefix, local-name(), $suffix)"/>
      </xsl:attribute>
      <xsl:value-of select="text()"/>
    </field>
  </xsl:for-each>

  <xsl:for-each select="//codeBook:citation/codeBook:prodStmt/codeBook:prodDate[normalize-space(text())]">
    <field>
      <xsl:attribute name="name">
        <xsl:value-of select="concat($prefix, local-name(), $suffix)"/>
      </xsl:attribute>
      <xsl:value-of select="text()"/>
    </field>
  </xsl:for-each>

  <xsl:for-each select="//codeBook:stdyInfo/codeBook:subject/codeBook:keyword[normalize-space(text())]">
    <field>
      <xsl:attribute name="name">
        <xsl:value-of select="concat($prefix, local-name(), $suffix)"/>
      </xsl:attribute>
      <xsl:value-of select="text()"/>
    </field>
  </xsl:for-each>

  <xsl:for-each select="//codeBook:stdyInfo/codeBook:abstract[normalize-space(text())]">
    <field>
      <xsl:attribute name="name">
        <xsl:value-of select="concat($prefix, local-name(), $suffix)"/>
      </xsl:attribute>
      <xsl:value-of select="text()"/>
    </field>
  </xsl:for-each>

  <xsl:for-each select="//codeBook:stdyInfo/codeBook:sumDscr/codeBook:geogCover[normalize-space(text())]">
    <field>
      <xsl:attribute name="name">
        <xsl:value-of select="concat($prefix, local-name(), $suffix)"/>
      </xsl:attribute>
      <xsl:value-of select="text()"/>
    </field>
  </xsl:for-each>

  <xsl:for-each select="//codeBook:stdyInfo/codeBook:sumDscr/codeBook:timePrd[normalize-space(text())]">
    <field>
      <xsl:attribute name="name">
        <xsl:value-of select="concat($prefix, local-name(), $suffix)"/>
      </xsl:attribute>
      <xsl:value-of select="text()"/>
    </field>
  </xsl:for-each>

  <xsl:for-each select="//codeBook:stdyInfo/codeBook:sumDscr/codeBook:dataKind[normalize-space(text())]">
    <field>
      <xsl:attribute name="name">
        <xsl:value-of select="concat($prefix, local-name(), $suffix)"/>
      </xsl:attribute>
      <xsl:value-of select="text()"/>
    </field>
  </xsl:for-each>

  <xsl:for-each select="//codeBook:method/codeBook:dataColl/codeBook:sources/codeBook:dataSrc[normalize-space(text())]">
    <field>
      <xsl:attribute name="name">
        <xsl:value-of select="concat($prefix, local-name(), $suffix)"/>
      </xsl:attribute>
      <xsl:value-of select="text()"/>
    </field>
  </xsl:for-each>

  <xsl:for-each select="//codeBook:dataAccs/codeBook:useStmt/codeBook:restrctn[normalize-space(text())]">
    <field>
      <xsl:attribute name="name">
        <xsl:value-of select="concat($prefix, local-name(), $suffix)"/>
      </xsl:attribute>
      <xsl:value-of select="text()"/>
    </field>
  </xsl:for-each>

  <xsl:for-each select="//codeBook:otherMat/codeBook:notes[normalize-space(text())]">
    <field>
      <xsl:attribute name="name">
        <xsl:value-of select="concat($prefix, local-name(), $suffix)"/>
      </xsl:attribute>
      <xsl:value-of select="text()"/>
    </field>
  </xsl:for-each>

  </xsl:template>

</xsl:stylesheet>