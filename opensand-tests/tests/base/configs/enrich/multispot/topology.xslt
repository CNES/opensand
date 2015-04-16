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

<xsl:template match="//sat_carrier/spot[@id='1']">
    <xsl:copy-of select="."/>
    <xsl:call-template name="Newline" />
    <spot id="2" gw="0">
    <xsl:call-template name="Newline" />
        <carriers>
    <xsl:call-template name="Newline" />
            <carrier id="10" type="ctrl_out"     ip_address="239.137.194.223" port="55010" ip_multicast="true"  />
    <xsl:call-template name="Newline" />
            <carrier id="11" type="ctrl_in"      ip_address="192.171.18.15"   port="55011" ip_multicast="false" />
    <xsl:call-template name="Newline" />
            <carrier id="12" type="logon_out"    ip_address="192.171.18.42"   port="55012" ip_multicast="false" />
    <xsl:call-template name="Newline" />
            <carrier id="13" type="logon_in"     ip_address="192.171.18.15"   port="55013" ip_multicast="false" />
    <xsl:call-template name="Newline" />
            <carrier id="14" type="data_out_st"  ip_address="239.137.194.224" port="55014" ip_multicast="true" />
    <xsl:call-template name="Newline" />
            <carrier id="15" type="data_in_st"   ip_address="192.171.18.15"   port="55015" ip_multicast="false" />
    <xsl:call-template name="Newline" />
            <carrier id="16" type="data_out_gw"  ip_address="192.171.18.42"   port="55016" ip_multicast="false" />
    <xsl:call-template name="Newline" />
            <carrier id="17" type="data_in_gw"   ip_address="192.171.18.15"   port="55017" ip_multicast="false" />
    <xsl:call-template name="Newline" />
        </carriers>
    <xsl:call-template name="Newline" />
    </spot>
    <xsl:call-template name="Newline" />
</xsl:template>


<xsl:template match="//spot_table/spot[@id='1']">
    <xsl:call-template name="Newline" />
    <spot id='1'>
    <xsl:call-template name="Newline" />
        <terminals>
    <xsl:call-template name="Newline" />
            <tal id='1' />
    <xsl:call-template name="Newline" />
        </terminals>
    <xsl:call-template name="Newline" />
    </spot>
    <xsl:call-template name="Newline" />
    <spot id="2">
    <xsl:call-template name="Newline" />
        <terminals>
    <xsl:call-template name="Newline" />
           <tal id="2" />
    <xsl:call-template name="Newline" />
        </terminals>
    <xsl:call-template name="Newline" />
    </spot>
</xsl:template>

</xsl:stylesheet>

