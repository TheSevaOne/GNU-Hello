/* Exponential function minus one.
   Copyright (C) 2011-2017 Free Software Foundation, Inc.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

#include <config.h>

/* Specification.  */
#include <math.h>

#if HAVE_SAME_LONG_DOUBLE_AS_DOUBLE

long double
expm1l (long double x)
{
  return expm1 (x);
}

#else

# include <float.h>

/* A value slightly larger than log(2).  */
#define LOG2_PLUS_EPSILON 0.6931471805599454L

/* Best possible approximation of log(2) as a 'long double'.  */
#define LOG2 0.693147180559945309417232121458176568075L

/* Best possible approximation of 1/log(2) as a 'long double'.  */
#define LOG2_INVERSE 1.44269504088896340735992468100189213743L

/* Best possible approximation of log(2)/256 as a 'long double'.  */
#define LOG2_BY_256 0.00270760617406228636491106297444600221904L

/* Best possible approximation of 256/log(2) as a 'long double'.  */
#define LOG2_BY_256_INVERSE 369.329930467574632284140718336484387181L

/* The upper 32 bits of log(2)/256.  */
#define LOG2_BY_256_HI_PART 0.0027076061733168899081647396087646484375L
/* log(2)/256 - LOG2_HI_PART.  */
#define LOG2_BY_256_LO_PART \
  0.000000000000745396456746323365681353781544922399845L

long double
expm1l (long double x)
{
  if (isnanl (x))
    return x;

  if (x >= (long double) LDBL_MAX_EXP * LOG2_PLUS_EPSILON)
    /* x > LDBL_MAX_EXP * log(2)
       hence exp(x) > 2^LDBL_MAX_EXP, overflows to Infinity.  */
    return HUGE_VALL;

  if (x <= (long double) (- LDBL_MANT_DIG) * LOG2_PLUS_EPSILON)
    /* x < (- LDBL_MANT_DIG) * log(2)
       hence 0 < exp(x) < 2^-LDBL_MANT_DIG,
       hence -1 < exp(x)-1 < -1 + 2^-LDBL_MANT_DIG
       rounds to -1.  */
    return -1.0L;

  if (x <= - LOG2_PLUS_EPSILON)
    /* 0 < exp(x) < 1/2.
       Just compute exp(x), then subtract 1.  */
    return expl (x) - 1.0L;

  if (x == 0.0L)
    /* Return a zero with the same sign as x.  */
    return x;

  /* Decompose x into
       x = n * log(2) + m * log(2)/256 + y
     where
       n is an integer, n >= -1,
       m is an integer, -128 <= m <= 128,
       y is a number, |y| <= log(2)/512 + epsilon = 0.00135...
     Then
       exp(x) = 2^n * exp(m * log(2)/256) * exp(y)
     Compute each factor minus one, then combine them through the
     formula (1+a)*(1+b) = 1 + (a+b*(1+a)),
     that is (1+a)*(1+b) - 1 = a + b*(1+a).
     The first factor is an ldexpl() call.
     The second factor is a table lookup.
     The third factor minus one is computed
     - either as sinh(y) + sinh(y)^2 / (cosh(y) + 1)
       where sinh(y) is computed through the power series:
         sinh(y) = y + y^3/3! + y^5/5! + ...
       and cosh(y) is computed as hypot(1, sinh(y)),
     - or as exp(2*z) - 1 = 2 * tanh(z) / (1 - tanh(z))
       where z = y/2
       and tanh(z) is computed through its power series:
         tanh(z) = z
                   - 1/3 * z^3
                   + 2/15 * z^5
                   - 17/315 * z^7
                   + 62/2835 * z^9
                   - 1382/155925 * z^11
                   + 21844/6081075 * z^13
                   - 929569/638512875 * z^15
                   + ...
       Since |z| <= log(2)/1024 < 0.0007, the relative contribution of the
       z^13 term is < 0.0007^12 < 2^-120 <= 2^-LDBL_MANT_DIG, therefore we
       can truncate the series after the z^11 term.

     Given the usual bounds LDBL_MAX_EXP <= 16384, LDBL_MANT_DIG <= 120, we
     can estimate x:  -84 <= x <= 11357.
     This means, when dividing x by log(2), where we want x mod log(2)
     to be precise to LDBL_MANT_DIG bits, we have to use an approximation
     to log(2) that has 14+LDBL_MANT_DIG bits.  */

  {
    long double nm = roundl (x * LOG2_BY_256_INVERSE); /* = 256 * n + m */
    /* n has at most 15 bits, nm therefore has at most 23 bits, therefore
       n * LOG2_HI_PART is computed exactly, and n * LOG2_LO_PART is computed
       with an absolute error < 2^15 * 2e-10 * 2^-LDBL_MANT_DIG.  */
    long double y_tmp = x - nm * LOG2_BY_256_HI_PART;
    long double y = y_tmp - nm * LOG2_BY_256_LO_PART;
    long double z = 0.5L * y;

/* Coefficients of the power series for tanh(z).  */
#define TANH_COEFF_1   1.0L
#define TANH_COEFF_3  -0.333333333333333333333333333333333333334L
#define TANH_COEFF_5   0.133333333333333333333333333333333333334L
#define TANH_COEFF_7  -0.053968253968253968253968253968253968254L
#define TANH_COEFF_9   0.0218694885361552028218694885361552028218L
#define TANH_COEFF_11 -0.00886323552990219656886323552990219656886L
#define TANH_COEFF_13  0.00359212803657248101692546136990581435026L
#define TANH_COEFF_15 -0.00145583438705131826824948518070211191904L

    long double z2 = z * z;
    long double tanh_z =
      (((((TANH_COEFF_11
           * z2 + TANH_COEFF_9)
          * z2 + TANH_COEFF_7)
         * z2 + TANH_COEFF_5)
        * z2 + TANH_COEFF_3)
       * z2 + TANH_COEFF_1)
      * z;

    long double exp_y_minus_1 = 2.0L * tanh_z / (1.0L - tanh_z);

    int n = (int) roundl (nm * (1.0L / 256.0L));
    int m = (int) nm - 256 * n;

    /* expm1l_table[i] = exp((i - 128) * log(2)/256) - 1.
       Computed in GNU clisp through
         (setf (long-float-digits) 128)
         (setq a 0L0)
         (setf (long-float-digits) 256)
         (dotimes (i 257)
           (format t "        ~D,~%"
                   (float (- (exp (* (/ (- i 128) 256) (log 2L0))) 1) a)))  */
    static const long double expm1l_table[257] =
      {
        -0.292893218813452475599155637895150960716L,
        -0.290976057839792401079436677742323809165L,
        -0.289053698915417220095325702647879950038L,
        -0.287126127947252846596498423285616993819L,
        -0.285193330804014994382467110862430046956L,
        -0.283255293316105578740250215722626632811L,
        -0.281312001275508837198386957752147486471L,
        -0.279363440435687168635744042695052413926L,
        -0.277409596511476689981496879264164547161L,
        -0.275450455178982509740597294512888729286L,
        -0.273486002075473717576963754157712706214L,
        -0.271516222799278089184548475181393238264L,
        -0.269541102909676505674348554844689233423L,
        -0.267560627926797086703335317887720824384L,
        -0.265574783331509036569177486867109287348L,
        -0.263583554565316202492529493866889713058L,
        -0.261586927030250344306546259812975038038L,
        -0.259584886088764114771170054844048746036L,
        -0.257577417063623749727613604135596844722L,
        -0.255564505237801467306336402685726757248L,
        -0.253546135854367575399678234256663229163L,
        -0.251522294116382286608175138287279137577L,
        -0.2494929651867872398674385184702356751864L,
        -0.247458134188296727960327722100283867508L,
        -0.24541778620328863011699022448340323429L,
        -0.243371906273695048903181511842366886387L,
        -0.24132047940089265059510885341281062657L,
        -0.239263490545592708236869372901757573532L,
        -0.237200924627730846574373155241529522695L,
        -0.23513276652635648805745654063657412692L,
        -0.233059001079521999099699248246140670544L,
        -0.230979613084171535783261520405692115669L,
        -0.228894587296029588193854068954632579346L,
        -0.226803908429489222568744221853864674729L,
        -0.224707561157500020438486294646580877171L,
        -0.222605530111455713940842831198332609562L,
        -0.2204977998810815164831359552625710592544L,
        -0.218384355014321147927034632426122058645L,
        -0.2162651800172235534675441445217774245016L,
        -0.214140259353829315375718509234297186439L,
        -0.212009577446056756772364919909047495547L,
        -0.209873118673587736597751517992039478005L,
        -0.2077308673737531349400659265343210916196L,
        -0.205582807841418027883101951185666435317L,
        -0.2034289243288665510313756784404656320656L,
        -0.201269201045686450868589852895683430425L,
        -0.199103622158653323103076879204523186316L,
        -0.196932171791614537151556053482436428417L,
        -0.19475483402537284591023966632129970827L,
        -0.192571592897569679960015418424270885733L,
        -0.190382432402568125350119133273631796029L,
        -0.188187336491335584102392022226559177731L,
        -0.185986289071326116575890738992992661386L,
        -0.183779274006362464829286135533230759947L,
        -0.181566275116517756116147982921992768975L,
        -0.17934727617799688564586793151548689933L,
        -0.1771222609230175777406216376370887771665L,
        -0.1748912130396911245164132617275148983224L,
        -0.1726541161719028012138814282020908791644L,
        -0.170410953919191957302175212789218768074L,
        -0.168161709836631782476831771511804777363L,
        -0.165906367434708746670203829291463807099L,
        -0.1636449101792017131905953879307692887046L,
        -0.161377321491060724103867675441291294819L,
        -0.15910358474628545696887452376678510496L,
        -0.15682368327580335203567701228614769857L,
        -0.154537600365347409013071332406381692911L,
        -0.152245319255333652509541396360635796882L,
        -0.149946823140738265249318713251248832456L,
        -0.147642095170974388162796469615281683674L,
        -0.145331118449768586448102562484668501975L,
        -0.143013876035036980698187522160833990549L,
        -0.140690350938761042185327811771843747742L,
        -0.138360526126863051392482883127641270248L,
        -0.136024384519081218878475585385633792948L,
        -0.133681908988844467561490046485836530346L,
        -0.131333082363146875502898959063916619876L,
        -0.128977887422421778270943284404535317759L,
        -0.126616306900415529961291721709773157771L,
        -0.1242483234840609219490048572320697039866L,
        -0.121873919813350258443919690312343389353L,
        -0.1194930784812080879189542126763637438278L,
        -0.11710578203336358947830887503073906297L,
        -0.1147120129682226132300120925687579825894L,
        -0.1123117537367393737247203999003383961205L,
        -0.1099049867422877955201404475637647649574L,
        -0.1074916943405325099278897180135900838485L,
        -0.1050718588392995019970556101123417014993L,
        -0.102645462498446406786148378936109092823L,
        -0.1002124875297324539725723033374854302454L,
        -0.097772916096688059846161368344495155786L,
        -0.0953267303144840657307406742107731280055L,
        -0.092873912249800621875082699818829828767L,
        -0.0904144439206957158520284361718212536293L,
        -0.0879483072964733445019372468353990225585L,
        -0.0854754842975513284540160873038416459095L,
        -0.0829959567953287682564584052058555719614L,
        -0.080509706612053141143695628825336081184L,
        -0.078016715520687037466429613329061550362L,
        -0.075516965244774535807472733052603963221L,
        -0.073010437458307215803773464831151680239L,
        -0.070497113785589807692349282254427317595L,
        -0.067976975801105477595185454402763710658L,
        -0.0654500050293807475554878955602008567352L,
        -0.06291618294485004933500052502277673278L,
        -0.0603754909717199109794126487955155117284L,
        -0.0578279104838327751561896480162548451191L,
        -0.055273422804530448266460732621318468453L,
        -0.0527120092065171793298906732865376926237L,
        -0.0501436509117223676387482401930039000769L,
        -0.0475683290911628981746625337821392744829L,
        -0.044986024864805103778829470427200864833L,
        -0.0423967193014263530636943648520845560749L,
        -0.0398003934184762630513928111129293882558L,
        -0.0371970281819375355214808849088086316225L,
        -0.0345866045061864160477270517354652168038L,
        -0.0319691032538527747009720477166542375817L,
        -0.0293445052356798073922893825624102948152L,
        -0.0267127912103833568278979766786970786276L,
        -0.0240739418845108520444897665995250062307L,
        -0.0214279379122998654908388741865642544049L,
        -0.018774759895536286618755114942929674984L,
        -0.016114388383412110943633198761985316073L,
        -0.01344680387238284353202993186779328685225L,
        -0.0107719868060245158708750409344163322253L,
        -0.00808991757489031507008688867384418356197L,
        -0.00540057651636682434752231377783368554176L,
        -0.00270394391452987374234008615207739887604L,
        0.0L,
        0.00271127505020248543074558845036204047301L,
        0.0054299011128028213513839559347998147001L,
        0.00815589811841751578309489081720103927357L,
        0.0108892860517004600204097905618605243881L,
        0.01363008495148943884025892906393992959584L,
        0.0163783149109530379404931137862940627635L,
        0.0191339960777379496848780958207928793998L,
        0.0218971486541166782344801347832994397821L,
        0.0246677928971356451482890762708149276281L,
        0.0274459491187636965388611939222137814994L,
        0.0302316376860410128717079024539045670944L,
        0.0330248790212284225001082839704609180866L,
        0.0358256936019571200299832090180813718441L,
        0.0386341019613787906124366979546397325796L,
        0.0414501246883161412645460790118931264803L,
        0.0442737824274138403219664787399290087847L,
        0.0471050958792898661299072502271122405627L,
        0.049944085800687266082038126515907909062L,
        0.0527907730046263271198912029807463031904L,
        0.05564517836055715880834132515293865216L,
        0.0585073227945126901057721096837166450754L,
        0.0613772272892620809505676780038837262945L,
        0.0642549128844645497886112570015802206798L,
        0.0671404006768236181695211209928091626068L,
        0.070033711820241773542411936757623568504L,
        0.0729348675259755513850354508738275853402L,
        0.0758438890627910378032286484760570740623L,
        0.0787607977571197937406800374384829584908L,
        0.081685614993215201942115594422531125645L,
        0.0846183622133092378161051719066143416095L,
        0.0875590609177696653467978309440397078697L,
        0.090507732665257659207010655760707978993L,
        0.0934643990728858542282201462504471620805L,
        0.096429081816376823386138295859248481766L,
        0.099401802630221985463696968238829904039L,
        0.1023825833078409435564142094256468575113L,
        0.1053714457017412555882746962569503110404L,
        0.1083684117236786380094236494266198501387L,
        0.111373503344817603850149254228916637444L,
        0.1143867425958925363088129569196030678004L,
        0.1174081515673691990545799630857802666544L,
        0.120437752409606684429003879866313012766L,
        0.1234755673330198007337297397753214319548L,
        0.1265216186082418997947986437870347776336L,
        0.12957592856628814599726498884024982591L,
        0.1326385195987192279870737236776230843835L,
        0.135709414157805514240390330676117013429L,
        0.1387886347566916537038302838415112547204L,
        0.14187620396956162271229760828788093894L,
        0.144972144431804219394413888222915895793L,
        0.148076478840179006778799662697342680031L,
        0.15118922995298270581775963520198253612L,
        0.154310420590216039548221528724806960684L,
        0.157440073633751029613085766293796821108L,
        0.160578212027498746369459472576090986253L,
        0.163724858777577513813573599092185312343L,
        0.166880036952481570555516298414089287832L,
        0.1700437696832501880802590357927385730016L,
        0.1732160801636372475348043545132453888896L,
        0.176396991650281276284645728483848641053L,
        0.1795865274628759454861005667694405189764L,
        0.182784710984341029924457204693850757963L,
        0.185991565660993831371265649534215563735L,
        0.189207115002721066717499970560475915293L,
        0.192431382583151222142727558145431011481L,
        0.1956643920398273745838370498654519757025L,
        0.1989061670743804817703025579763002069494L,
        0.202156731452703142096396957497765876L,
        0.205416109005123825604211432558411335666L,
        0.208684323626581577354792255889216998483L,
        0.211961399276801194468168917732493045449L,
        0.2152473599804688781165202513387984576236L,
        0.218542229827408361758207148117394510722L,
        0.221846032972757516903891841911570785834L,
        0.225158793637145437709464594384845353705L,
        0.2284805361068700056940089577927818403626L,
        0.231811284734075935884556653212794816605L,
        0.235151063936933305692912507415415760296L,
        0.238499898199816567833368865859612431546L,
        0.241857812073484048593677468726595605511L,
        0.245224830175257932775204967486152674173L,
        0.248600977189204736621766097302495545187L,
        0.251986277866316270060206031789203597321L,
        0.255380757024691089579390657442301194598L,
        0.258784439549716443077860441815162618762L,
        0.262197350394250708014010258518416459672L,
        0.265619514578806324196273999873453036297L,
        0.269050957191733222554419081032338004715L,
        0.272491703389402751236692044184602176772L,
        0.27594177839639210038120243475928938891L,
        0.279401207505669226913587970027852545961L,
        0.282870016078778280726669781021514051111L,
        0.286348229546025533601482208069738348358L,
        0.289835873406665812232747295491552189677L,
        0.293332973229089436725559789048704304684L,
        0.296839554651009665933754117792451159835L,
        0.300355643379650651014140567070917791291L,
        0.303881265191935898574523648951997368331L,
        0.30741644593467724479715157747196172848L,
        0.310961211524764341922991786330755849366L,
        0.314515587949354658485983613383997794966L,
        0.318079601266063994690185647066116617661L,
        0.321653277603157514326511812330609226158L,
        0.325236643159741294629537095498721674113L,
        0.32882972420595439547865089632866510792L,
        0.33243254708316144935164337949073577407L,
        0.336045138204145773442627904371869759286L,
        0.339667524053303005360030669724352576023L,
        0.343299731186835263824217146181630875424L,
        0.346941786232945835788173713229537282073L,
        0.350593715892034391408522196060133960038L,
        0.354255546936892728298014740140702804344L,
        0.357927306212901046494536695671766697444L,
        0.361609020638224755585535938831941474643L,
        0.365300717204011815430698360337542855432L,
        0.369002422974590611929601132982192832168L,
        0.372714165087668369284997857144717215791L,
        0.376435970754530100216322805518686960261L,
        0.380167867260238095581945274358283464698L,
        0.383909881963831954872659527265192818003L,
        0.387662042298529159042861017950775988895L,
        0.391424375771926187149835529566243446678L,
        0.395196909966200178275574599249220994717L,
        0.398979672538311140209528136715194969206L,
        0.402772691220204706374713524333378817108L,
        0.40657599381901544248361973255451684411L,
        0.410389608217270704414375128268675481146L,
        0.414213562373095048801688724209698078569L
      };

    long double t = expm1l_table[128 + m];

    /* (1+t) * (1+exp_y_minus_1) - 1 = t + (1+t)*exp_y_minus_1 */
    long double p_minus_1 = t + (1.0L + t) * exp_y_minus_1;

    long double s = ldexpl (1.0L, n) - 1.0L;

    /* (1+s) * (1+p_minus_1) - 1 = s + (1+s)*p_minus_1 */
    return s + (1.0L + s) * p_minus_1;
  }
}

#endif
