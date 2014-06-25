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

<xsl:template match="lan_adaptation_schemes">
        <lan_adaptation_schemes>
    <xsl:call-template name="Newline" />
            <lan_scheme pos="0" proto="Ethernet" />
    <xsl:call-template name="Newline" />
        </lan_adaptation_schemes>
</xsl:template>


</xsl:stylesheet>

