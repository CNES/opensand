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

<xsl:template match="//forward_down_band/spot[@id='1']/carriers_distribution">
    <carriers_distribution>
    <xsl:call-template name="Newline" />
        <down_carriers access_type="VCM" category="Standard" ratio="1,9" symbol_rate="40E6" fmt_group="1,2"/>
    <xsl:call-template name="Newline" />
    </carriers_distribution>
</xsl:template>

<xsl:template match="//forward_down_band/spot/fmt_groups">
    <fmt_groups>
    <xsl:call-template name="Newline" />
            <group id="1" fmt_id="28" />
    <xsl:call-template name="Newline" />
            <group id="2" fmt_id="27" />
    <xsl:call-template name="Newline" />
    </fmt_groups>
</xsl:template>


</xsl:stylesheet>

