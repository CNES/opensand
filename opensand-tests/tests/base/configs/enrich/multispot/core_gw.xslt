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

<xsl:template match="//dvb_ncc/spot[@id='1']">
    <xsl:copy-of select="."/>
    <xsl:call-template name="Newline" />
    <spot id="2">
    <xsl:call-template name="Newline" />
        <simulation>none</simulation>
    <xsl:call-template name="Newline" />
        <simu_file>/etc/opensand/simulation/dama_spot2.input</simu_file>
    <xsl:call-template name="Newline" />
        <simu_random>10:100:1024:55:200:100</simu_random>
    <xsl:call-template name="Newline" />
        <event_file>none</event_file>
    <xsl:call-template name="Newline" />
        <layer2_fifos>
    <xsl:call-template name="Newline" />
            <fifo priority="0" name="NM" size_max="1000" access_type="ACM" />
    <xsl:call-template name="Newline" />
            <fifo priority="1" name="EF" size_max="3000" access_type="ACM" />
    <xsl:call-template name="Newline" />
            <fifo priority="2" name="SIG" size_max="1000" access_type="ACM" />
    <xsl:call-template name="Newline" />
            <fifo priority="3" name="AF" size_max="2000" access_type="ACM" /> 
    <xsl:call-template name="Newline" />
            <fifo priority="4" name="BE" size_max="6000" access_type="ACM" /> 
    <xsl:call-template name="Newline" />
        </layer2_fifos>
    <xsl:call-template name="Newline" />
    </spot>
    <xsl:call-template name="Newline" />
</xsl:template>


<xsl:template match="//slotted_aloha/spot[@id='1']">
    <xsl:copy-of select="."/>
    <xsl:call-template name="Newline" />
   <spot id="2">
    <xsl:call-template name="Newline" />
        <algorithm>CRDSA</algorithm>
    <xsl:call-template name="Newline" />
        <simulation_traffic>
    <xsl:call-template name="Newline" />
            <simu category="Standard" nb_max_packets="0" nb_replicas="2" ratio="20" />
    <xsl:call-template name="Newline" />
        </simulation_traffic>
    <xsl:call-template name="Newline" />
    </spot>
    <xsl:call-template name="Newline" />
</xsl:template>
    


</xsl:stylesheet>

