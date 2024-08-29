library IEEE;
use IEEE.STD_LOGIC_1164.ALL;
use IEEE.NUMERIC_STD.ALL;

entity RGB2Gray is
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
        
        -- AXI Stream output
        m_axis_tvalid : out std_logic;
        m_axis_tready : in  std_logic;
        m_axis_tdata  : out std_logic_vector(7 downto 0);
        m_axis_tlast  : out std_logic;
        m_axis_tuser  : out std_logic
    );
end RGB2Gray;

architecture Behavioral of RGB2Gray is
    signal r, g, b : std_logic_vector(7 downto 0);
    signal gray    : std_logic_vector(15 downto 0);
begin
    process(clk, rst)
    begin
        if rst = '0' then
            s_axis_tready <= '0';
            m_axis_tvalid <= '0';
            m_axis_tdata  <= (others => '0');
            m_axis_tlast  <= '0';
            m_axis_tuser  <= '0';
        elsif rising_edge(clk) then
            if s_axis_tvalid = '1' and m_axis_tready = '1' then
                -- Extraire les composantes RGB
                r <= s_axis_tdata(23 downto 16);
                g <= s_axis_tdata(15 downto 8);
                b <= s_axis_tdata(7 downto 0);

                -- Conversion RGB vers niveaux de gris
                -- Utilisation de la formule : Gray = 0.299*R + 0.587*G + 0.114*B
                gray <= std_logic_vector(
                    unsigned(r) * 76 / 256 + 
                    unsigned(g) * 150 / 256 + 
                    unsigned(b) * 29 / 256
                );

                -- Passer les données converties au flux de sortie
                m_axis_tdata  <= gray(7 downto 0);
                m_axis_tvalid <= '1';
                m_axis_tlast  <= s_axis_tlast;
                m_axis_tuser  <= s_axis_tuser;  -- Propager tuser signal
            else
                m_axis_tvalid <= '0';
            end if;

            -- Prêt à recevoir les données
            s_axis_tready <= m_axis_tready;
        end if;
    end process;
end Behavioral;
