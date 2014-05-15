<?xml version="1.0" encoding="UTF-8"?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0" 
    xmlns:ddi="ddi:codebook:2_5"
    xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance">
  <xsl:output method="xml" version="1.0" encoding="UTF-8" indent="yes" media-type="text/xml"/>
  <!-- Remove leading / trailing / repeating white space from all elements -->
  <xsl:strip-space elements="*"/>
  <!-- Remove empty elements -->
  <xsl:template match="*[not(node())]"/>
  <!-- Identity transform for only those nodes / attributes which are not empty -->
  <xsl:template match="@*|node()">
    <xsl:copy>
      <xsl:apply-templates select="@*[normalize-space()]|node()[normalize-space()]"/>
    </xsl:copy>
  </xsl:template>
  
  <!-- 
    Currently this XSLT only works for the a specific DDI form that's part of this
    module. DDI Final.xml If that form changes this XSLT must change as well.
    -->
  
  <!-- Reorder ddi:codeBook child elements -->
  <xsl:template match="ddi:codeBook">
    <xsl:copy>
      <xsl:apply-templates select="@*"/>
      <xsl:apply-templates select="ddi:stdyDscr"/>
      <xsl:apply-templates select="ddi:fileDscr"/>
      <xsl:apply-templates select="ddi:otherMat"/>
    </xsl:copy>
  </xsl:template>
  
  <!-- Reorder ddi:stdyDscr child elements -->
  <xsl:template match="ddi:stdyDscr">
    <xsl:copy>
      <xsl:apply-templates select="ddi:citation"/>
      <xsl:apply-templates select="ddi:stdyInfo"/>
      <xsl:apply-templates select="ddi:method"/>
      <xsl:apply-templates select="ddi:dataAccs"/>
      <xsl:apply-templates select="ddi:notes"/>
    </xsl:copy>
  </xsl:template>
  
  <!-- Reorder ddi:citation child elements -->
  <xsl:template match="ddi:citation">
    <xsl:copy>
      <xsl:apply-templates select="ddi:titlStmt"/>
      <xsl:apply-templates select="ddi:rspStmt"/>
      <xsl:apply-templates select="ddi:prodStmt"/>
      <xsl:apply-templates select="ddi:distStmt"/>
      <xsl:apply-templates select="ddi:verStmt"/>
      <xsl:apply-templates select="ddi:biblCit"/>
      <xsl:apply-templates select="ddi:notes"/>      
    </xsl:copy>
  </xsl:template>
  
  <!-- Reorder ddi:titlStmt child elements -->
  <xsl:template match="ddi:titlStmt">
    <xsl:copy>
      <xsl:apply-templates select="ddi:titl"/>
      <xsl:apply-templates select="ddi:IDNo"/>
    </xsl:copy>
  </xsl:template>
  
  <!-- Reorder ddi:rspStmt child elements -->
  <xsl:template match="ddi:rspStmt">
    <xsl:copy>
      <xsl:apply-templates select="ddi:AuthEnty"/>
      <xsl:apply-templates select="ddi:othId"/>
    </xsl:copy>
  </xsl:template>
  
  <!-- Reorder ddi:prodStmt child elements -->
  <xsl:template match="ddi:prodStmt">
    <xsl:copy>
      <xsl:apply-templates select="ddi:copyright"/>
      <xsl:apply-templates select="ddi:prodDate"/>
      <xsl:apply-templates select="ddi:software"/>
      <xsl:apply-templates select="ddi:fundAg"/>
    </xsl:copy>
  </xsl:template>
  
  <!-- Reorder ddi:stdyInfo child elements -->
  <xsl:template match="ddi:stdyInfo">
    <xsl:copy>
      <xsl:apply-templates select="ddi:subject"/>
      <xsl:apply-templates select="ddi:abstract"/>
      <xsl:apply-templates select="ddi:sumDscr"/>
    </xsl:copy>
  </xsl:template>
  
  <!-- Reorder ddi:subject child elements -->
  <xsl:template match="ddi:subject">
    <xsl:copy>
      <xsl:apply-templates select="ddi:keyword"/>
      <xsl:apply-templates select="ddi:topcClas"/>
    </xsl:copy>
  </xsl:template>
  
  <!-- Reorder ddi:sumDscr child elements -->
  <xsl:template match="ddi:sumDscr">
    <xsl:copy>
      <timePrd>
      	<xsl:for-each select="*[starts-with(name(), 'time')]">
      	  <xsl:value-of select="string(.)"/>
          <xsl:if test="not(position()=last())">
            <xsl:text> -- </xsl:text>
          </xsl:if>
        </xsl:for-each>
      <timePrd>
      <xsl:apply-templates select="ddi:collDate"/>
      <xsl:apply-templates select="ddi:geogCover"/>
      <xsl:apply-templates select="ddi:dataKind"/>
    </xsl:copy>
  </xsl:template>
  
  <!-- Reorder ddi:method child elements -->
  <xsl:template match="ddi:dataColl">
    <xsl:copy>
      <xsl:apply-templates select="ddi:collMode"/>
      <xsl:apply-templates select="ddi:sources"/>
    </xsl:copy>
  </xsl:template>
 
</xsl:stylesheet>
