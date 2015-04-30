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

<xsl:template match="//return_up_band/spot/carriers_distribution">
    <carriers_distribution>
    <xsl:call-template name="Newline" />
        <up_carriers access_type="SCPC" category="Standard" ratio="10" symbol_rate="1E6" fmt_group="1"/>
    <xsl:call-template name="Newline" />
        <up_carriers access_type="DAMA" category="Standard" ratio="10" symbol_rate="1E6" fmt_group="1"/>
    <xsl:call-template name="Newline" />
    </carriers_distribution>
</xsl:template>

 <xsl:template match="//return_up_band/spot/tal_affectations">
    <tal_affectations>
    <xsl:call-template name="Newline" />
        <tal_affectation tal_id="1" category="Standard" />
     <xsl:call-template name="Newline" />
        <tal_affectation tal_id="2" category="Standard" />
    <xsl:call-template name="Newline" />
    </tal_affectations>
 </xsl:template>
 
 <xsl:template match="//return_up_band/spot/fmt_groups">
    <fmt_groups>
    <xsl:call-template name="Newline" />
         <group id="1" fmt_id="28" />
    <xsl:call-template name="Newline" />
        <group id="2" fmt_id="7" />
    <xsl:call-template name="Newline" />
    </fmt_groups>
 </xsl:template>

 
</xsl:stylesheet>

