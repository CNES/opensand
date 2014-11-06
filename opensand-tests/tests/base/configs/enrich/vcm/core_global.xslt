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

<xsl:template match="//forward_down_band/carriers_distribution">
    <carriers_distribution>
    <xsl:call-template name="Newline" />
        <down_carriers access_type="VCM" category="Standard" ratio="2,8" symbol_rate="28.8E6" fmt_group="1,1"/>
    <xsl:call-template name="Newline" />
    </carriers_distribution>
</xsl:template>


</xsl:stylesheet>

