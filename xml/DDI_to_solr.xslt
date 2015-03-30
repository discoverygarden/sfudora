<?xml version="1.0" encoding="UTF-8"?>
<!-- Basic MODS 
  This has been deprecated in favor of the slurp all MODS-->
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
    <!-- Create fields for the set of selected elements, named according to the 'local-name' and containing the 'text' -->
    <xsl:for-each select="./*">

      <field>
        <xsl:attribute name="name">
          <xsl:value-of select="concat($prefix, local-name(), $suffix)"/>
        </xsl:attribute>
        <xsl:value-of select="text()"/>
      </field>
    </xsl:for-each>

  </xsl:template>

</xsl:stylesheet>