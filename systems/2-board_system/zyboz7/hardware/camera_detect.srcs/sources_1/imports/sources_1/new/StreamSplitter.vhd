library IEEE;
use IEEE.STD_LOGIC_1164.ALL;
use IEEE.NUMERIC_STD.ALL;

entity StreamSplitter is
    Port (
        -- Clock and Reset
        clk          : in  std_logic;
        rst          : in  std_logic;
        
        -- AXI Stream input
        s_axis_tvalid : in  std_logic;
        s_axis_tready : out std_logic;
        s_axis_tdata  : in  std_logic_vector(23 downto 0);
        s_axis_tlast  : in  std_logic;
        s_axis_tuser  : in  std_logic;
        
        -- AXI Stream output 0
        m_axis_tvalid_0 : out std_logic;
        m_axis_tready_0 : in  std_logic;
        m_axis_tdata_0  : out std_logic_vector(23 downto 0);
        m_axis_tlast_0  : out std_logic;
        m_axis_tuser_0  : out std_logic;
        
        -- AXI Stream output 1
        m_axis_tvalid_1 : out std_logic;
        m_axis_tready_1 : in  std_logic;
        m_axis_tdata_1  : out std_logic_vector(23 downto 0);
        m_axis_tlast_1  : out std_logic;
        m_axis_tuser_1  : out std_logic
    );
end StreamSplitter;

architecture Behavioral of StreamSplitter is
begin
    process(clk, rst)
    begin
        if rst = '0' then
            s_axis_tready <= '0';
            
            m_axis_tvalid_0 <= '0';
            m_axis_tdata_0  <= (others => '0');
            m_axis_tlast_0  <= '0';
            m_axis_tuser_0  <= '0';
            
            m_axis_tvalid_1 <= '0';
            m_axis_tdata_1  <= (others => '0');
            m_axis_tlast_1  <= '0';
            m_axis_tuser_1  <= '0';
        elsif rising_edge(clk) then  
            m_axis_tdata_0  <= s_axis_tdata;
            m_axis_tvalid_0 <= '1';
            m_axis_tlast_0  <= s_axis_tlast;
            m_axis_tuser_0  <= s_axis_tuser;  -- Propager tuser signal
            
            m_axis_tdata_1  <= s_axis_tdata;
            m_axis_tvalid_1 <= '1';
            m_axis_tlast_1  <= s_axis_tlast;
            m_axis_tuser_1  <= s_axis_tuser;  -- Propager tuser signal

            -- Prêt à recevoir les données
            s_axis_tready <= m_axis_tready_0 and m_axis_tready_1;
        end if;
    end process;
end Behavioral;
