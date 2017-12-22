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

<xsl:template match="satellite_type">
    <satellite_type>regenerative</satellite_type>
</xsl:template>

<xsl:template match="return_link_standard">
    <return_link_standard>DVB-RCS2</return_link_standard>
</xsl:template>

<xsl:template match="//return_up_band/spot/fmt_groups">
    <fmt_groups>
    <xsl:call-template name="Newline" />
        <group id="1" fmt_id="3-12" />
    <xsl:call-template name="Newline" />
        <group id="2" fmt_id="12" />
    <xsl:call-template name="Newline" />
    </fmt_groups>
</xsl:template>

<xsl:template match="return_up_encap_schemes">
        <return_up_encap_schemes>
    <xsl:call-template name="Newline" />
                <encap_scheme pos="0" encap="RLE" />
    <xsl:call-template name="Newline" />
        </return_up_encap_schemes>
</xsl:template>


<xsl:template match="forward_down_encap_schemes">
        <forward_down_encap_schemes>
    <xsl:call-template name="Newline" />
                <encap_scheme pos="0" encap="RLE" />
    <xsl:call-template name="Newline" />
        </forward_down_encap_schemes>
</xsl:template>


</xsl:stylesheet>

