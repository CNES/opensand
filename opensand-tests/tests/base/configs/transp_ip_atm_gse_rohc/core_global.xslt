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
    <satellite_type>transparent</satellite_type>
</xsl:template>

<xsl:template match="up_return_encap_schemes">
        <up_return_encap_schemes>
    <xsl:call-template name="Newline" />
                <encap_scheme pos="0" encap="AAL5/ATM" />
    <xsl:call-template name="Newline" />
        </up_return_encap_schemes>
</xsl:template>


<xsl:template match="down_forward_encap_schemes">
        <down_forward_encap_schemes>
    <xsl:call-template name="Newline" />
                <encap_scheme pos="0" encap="GSE" />
    <xsl:call-template name="Newline" />
        </down_forward_encap_schemes>
</xsl:template>


</xsl:stylesheet>

