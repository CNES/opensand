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


<!-- For Slotted Aloha scenario we need to increase maximum number of packets -->
<xsl:template match="//slotted_aloha/nb_max_packets">
    <nb_max_packets>20</nb_max_packets>
</xsl:template>


</xsl:stylesheet>

