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

<xsl:template match="//return_up_band/spot[@id='1' and @gw='0']">
    <xsl:copy-of select="."/>
    <xsl:call-template name="Newline" />
    <spot id="2" gw="0">
    <xsl:call-template name="Newline" />
        <bandwidth>20</bandwidth>
    <xsl:call-template name="Newline" />
        <carriers_distribution>
    <xsl:call-template name="Newline" />
            <up_carriers access_type="DAMA" category="Standard" ratio="10" symbol_rate="7.4E6" fmt_group="1" /> 
    <xsl:call-template name="Newline" />
        </carriers_distribution>
    <xsl:call-template name="Newline" />
        <tal_affectations>
    <xsl:call-template name="Newline" />
            <tal_affectation tal_id="2" category="Standard" />
    <xsl:call-template name="Newline" />
        </tal_affectations>
    <xsl:call-template name="Newline" />
        <tal_default_affectation>Standard</tal_default_affectation>
    <xsl:call-template name="Newline" />
        <fmt_groups>
    <xsl:call-template name="Newline" />
            <group id="1" fmt_id="7" />
    <xsl:call-template name="Newline" />
        </fmt_groups>
    <xsl:call-template name="Newline" />
    </spot>
    <xsl:call-template name="Newline" />
</xsl:template>

<xsl:template match="//return_up_band/spot[@id='1' and @gw='0']/tal_affectations/tal_affectation[@tal_id='2']">
    <xsl:call-template name="Newline" />
            <tal_affectation tal_id="3" category="Standard" />
    <xsl:call-template name="Newline" />
</xsl:template>

<xsl:template match="//forward_down_band/spot[@id='1' and @gw='0']">
    <xsl:copy-of select="."/>
    <xsl:call-template name="Newline" />
    <spot id="2" gw="0">
    <xsl:call-template name="Newline" />
        <bandwidth>50</bandwidth>
    <xsl:call-template name="Newline" />
        <carriers_distribution>
    <xsl:call-template name="Newline" />
            <down_carriers access_type="ACM" category="Standard" ratio="10" symbol_rate="40E6" fmt_group="1" /> 
    <xsl:call-template name="Newline" />
        </carriers_distribution>
    <xsl:call-template name="Newline" />
        <tal_affectations>
    <xsl:call-template name="Newline" />
            <tal_affectation tal_id="2" category="Standard" />
    <xsl:call-template name="Newline" />
        </tal_affectations>
    <xsl:call-template name="Newline" />
        <tal_default_affectation>Standard</tal_default_affectation>
    <xsl:call-template name="Newline" />
        <fmt_groups>
    <xsl:call-template name="Newline" />
            <group id="1" fmt_id="28" />
    <xsl:call-template name="Newline" />
        </fmt_groups>
    <xsl:call-template name="Newline" />
    </spot>
    <xsl:call-template name="Newline" />
</xsl:template>

<xsl:template match="//physical_layer/spot[@id='1' and @gw='0']">
    <xsl:copy-of select="."/>
    <xsl:call-template name="Newline" />
    <spot id="2" gw="0">
    <xsl:call-template name="Newline" />
        <forward_down_modcod_time_series>/etc/opensand/modcod/forward_down_time_series_spot2_gw0.txt</forward_down_modcod_time_series>
    <xsl:call-template name="Newline" />
        <return_up_modcod_time_series>/etc/opensand/modcod/return_up_time_series_spot2_gw0.txt</return_up_modcod_time_series>
    <xsl:call-template name="Newline" />
        <loop_on_file>false</loop_on_file>
    <xsl:call-template name="Newline" />
    </spot>
    <xsl:call-template name="Newline" />
</xsl:template>

</xsl:stylesheet>

