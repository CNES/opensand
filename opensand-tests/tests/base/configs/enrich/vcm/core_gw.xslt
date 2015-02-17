<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0">
<xsl:output method="xml" indent="yes" encoding="UTF-8"/>

<xsl:template name="Newline">
<xsl:text>
        </xsl:text>
</xsl:template>

<xsl:template match="@*|node()">
    <xsl:copy>
        <xsl:apply-templates select="@*|node()"/>
    </xsl:copy> 
</xsl:template>

<xsl:template match="layer2_fifos">
    <layer2_fifos>
    <xsl:call-template name="Newline" />
        <fifo priority="4" name="BE" size_max="6000" access_type="VCM1" />
    <xsl:call-template name="Newline" />
    </layer2_fifos>
</xsl:template>


</xsl:stylesheet>

