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

<xsl:template match="is_scpc">
    <is_scpc>true</is_scpc>
</xsl:template>

<xsl:template match="scpc">
    <scpc>
    <xsl:call-template name="Newline" />
        <scpc_carrier_duration>5</scpc_carrier_duration>
    <xsl:call-template name="Newline" />
    </scpc>
</xsl:template>


</xsl:stylesheet>

