/* rijndael.js      Rijndael Reference Implementation
   Copyright (c) 2001 Fritz Schneider

 This software is provided as-is, without express or implied warranty.
 Permission to use, copy, modify, distribute or sell this software, with or
 without fee, for any purpose and by any individual or organization, is hereby
 granted, provided that the above copyright notice and this paragraph appear
 in all copies. Distribution as a part of an application or binary must
 include the above copyright notice in the documentation and/or other materials
 provided with the application or distribution.

 Note that the following code is a compressed version of Fritz's code
 and is only about one third the size of his original
 compressed version courtesy of Stephen Chapman http://javascript.about.com/
 */function cSL(e,t){var n=e.slice(0,t);e=e.slice(t).concat(n);return e}function XT(e){e<<=1;return e&256?e^283:e}function GF(e,t){var n,r=0;for(n=1;n<256;n*=2,t=XT(t))e&n&&(r^=t);return r}function bS(e,t){var n;t=="e"?n=SB:n=SBI;for(var r=0;r<4;r++)for(var i=0;i<Nb;i++)e[r][i]=n[e[r][i]]}function sR(e,t){for(var n=1;n<4;n++)t=="e"?e[n]=cSL(e[n],SO[Nb][n]):e[n]=cSL(e[n],Nb-SO[Nb][n])}function mC(e,t){var n=[];for(var r=0;r<Nb;r++){for(var i=0;i<4;i++)t=="e"?n[i]=GF(e[i][r],2)^GF(e[(i+1)%4][r],3)^e[(i+2)%4][r]^e[(i+3)%4][r]:n[i]=GF(e[i][r],14)^GF(e[(i+1)%4][r],11)^GF(e[(i+2)%4][r],13)^GF(e[(i+3)%4][r],9);for(var i=0;i<4;i++)e[i][r]=n[i]}}function aRK(e,t){for(var n=0;n<Nb;n++){e[0][n]^=t[n]&255;e[1][n]^=t[n]>>8&255;e[2][n]^=t[n]>>16&255;e[3][n]^=t[n]>>24&255}}function YE(e){var t=[],n;Nk=BS/32;Nb=BB/32;Nr=RA[Nk][Nb];for(var r=0;r<Nk;r++)t[r]=e[4*r]|e[4*r+1]<<8|e[4*r+2]<<16|e[4*r+3]<<24;for(r=Nk;r<Nb*(Nr+1);r++){n=t[r-1];r%Nk==0?n=(SB[n>>8&255]|SB[n>>16&255]<<8|SB[n>>24&255]<<16|SB[n&255]<<24)^RC[Math.floor(r/Nk)-1]:Nk>6&&r%Nk==4&&(n=SB[n>>24&255]<<24|SB[n>>16&255]<<16|SB[n>>8&255]<<8|SB[n&255]);t[r]=t[r-Nk]^n}return t}function Rd(e,t){bS(e,"e");sR(e,"e");mC(e,"e");aRK(e,t)}function iRd(e,t){aRK(e,t);mC(e,"d");sR(e,"d");bS(e,"d")}function FRd(e,t){bS(e,"e");sR(e,"e");aRK(e,t)}function iFRd(e,t){aRK(e,t);sR(e,"d");bS(e,"d")}function encrypt(e,t){var n;if(!e||e.length*8!=BB)return;if(!t)return;e=pB(e);aRK(e,t);for(n=1;n<Nr;n++)Rd(e,t.slice(Nb*n,Nb*(n+1)));FRd(e,t.slice(Nb*Nr));return uPB(e)}function decrypt(e,t){var n;if(!e||e.length*8!=BB)return;if(!t)return;e=pB(e);iFRd(e,t.slice(Nb*Nr));for(n=Nr-1;n>0;n--)iRd(e,t.slice(Nb*n,Nb*(n+1)));aRK(e,t);return uPB(e)}function byteArrayToString(e){var t="";for(var n=0;n<e.length;n++)e[n]!=0&&(t+=String.fromCharCode(e[n]));return t}function byteArrayToHex(e){var t="";if(!e)return;for(var n=0;n<e.length;n++)t+=(e[n]<16?"0":"")+e[n].toString(16);return t}function hexToByteArray(e){var t=[];if(e.length%2)return;if(e.indexOf("0x")==0||e.indexOf("0X")==0)e=e.substring(2);for(var n=0;n<e.length;n+=2)t[Math.floor(n/2)]=parseInt(e.slice(n,n+2),16);return t}function pB(e){var t=[];if(!e||e.length%4)return;t[0]=[];t[1]=[];t[2]=[];t[3]=[];for(var n=0;n<e.length;n+=4){t[0][n/4]=e[n];t[1][n/4]=e[n+1];t[2][n/4]=e[n+2];t[3][n/4]=e[n+3]}return t}function uPB(e){var t=[];for(var n=0;n<e[0].length;n++){t[t.length]=e[0][n];t[t.length]=e[1][n];t[t.length]=e[2][n];t[t.length]=e[3][n]}return t}function fPT(e){var t=BB/8,n;if(typeof e=="string"||e.indexOf){e=e.split("");for(n=0;n<e.length;n++)e[n]=e[n].charCodeAt(0)&255}for(n=t-e.length%t;n>0&&n<t;n--)e[e.length]=0;return e}function gRB(e){var t,n=[];for(t=0;t<e;t++)n[t]=Math.round(Math.random()*255);return n}function rijndaelEncrypt(e,t,n){var r,i,s,o=BB/8,u;if(!e||!t)return;if(t.length*8!=BS)return;if(n=="CBC")u=gRB(o);else{n="ECB";u=[]}e=fPT(e);r=YE(t);for(var a=0;a<e.length/o;a++){s=e.slice(a*o,(a+1)*o);if(n=="CBC")for(var i=0;i<o;i++)s[i]^=u[a*o+i];u=u.concat(encrypt(s,r))}return u}function rijndaelDecrypt(e,t,n){var r,i=BB/8,s=[],o,u;if(!e||!t||typeof e=="string")return;if(t.length*8!=BS)return;n||(n="ECB");r=YE(t);for(u=e.length/i-1;u>0;u--){o=decrypt(e.slice(u*i,(u+1)*i),r);if(n=="CBC")for(var a=0;a<i;a++)s[(u-1)*i+a]=o[a]^e[(u-1)*i+a];else s=o.concat(s)}n=="ECB"&&(s=decrypt(e.slice(0,i),r).concat(s));return s}function stringToByteArray(e){var t=[];for(var n=0;n<e.length;n++)t[n]=e.charCodeAt(n);return t}function genkey(){var e="";for(;;){var t=Math.random().toString();e+=t.substring(t.lastIndexOf(".")+1);if(e.length>31)return e.substring(0,32)}}var BS=128,BB=128,RA=[,,,,[,,,,10,,12,,14],,[,,,,12,,12,,14],,[,,,,14,,14,,14]],SO=[,,,,[,1,2,3],,[,1,2,3],,[,1,3,4]],RC=[1,2,4,8,16,32,64,128,27,54,108,216,171,77,154,47,94,188,99,198,151,53,106,212,179,125,250,239,197,145],SB=[99,124,119,123,242,107,111,197,48,1,103,43,254,215,171,118,202,130,201,125,250,89,71,240,173,212,162,175,156,164,114,192,183,253,147,38,54,63,247,204,52,165,229,241,113,216,49,21,4,199,35,195,24,150,5,154,7,18,128,226,235,39,178,117,9,131,44,26,27,110,90,160,82,59,214,179,41,227,47,132,83,209,0,237,32,252,177,91,106,203,190,57,74,76,88,207,208,239,170,251,67,77,51,133,69,249,2,127,80,60,159,168,81,163,64,143,146,157,56,245,188,182,218,33,16,255,243,210,205,12,19,236,95,151,68,23,196,167,126,61,100,93,25,115,96,129,79,220,34,42,144,136,70,238,184,20,222,94,11,219,224,50,58,10,73,6,36,92,194,211,172,98,145,149,228,121,231,200,55,109,141,213,78,169,108,86,244,234,101,122,174,8,186,120,37,46,28,166,180,198,232,221,116,31,75,189,139,138,112,62,181,102,72,3,246,14,97,53,87,185,134,193,29,158,225,248,152,17,105,217,142,148,155,30,135,233,206,85,40,223,140,161,137,13,191,230,66,104,65,153,45,15,176,84,187,22],SBI=[82,9,106,213,48,54,165,56,191,64,163,158,129,243,215,251,124,227,57,130,155,47,255,135,52,142,67,68,196,222,233,203,84,123,148,50,166,194,35,61,238,76,149,11,66,250,195,78,8,46,161,102,40,217,36,178,118,91,162,73,109,139,209,37,114,248,246,100,134,104,152,22,212,164,92,204,93,101,182,146,108,112,72,80,253,237,185,218,94,21,70,87,167,141,157,132,144,216,171,0,140,188,211,10,247,228,88,5,184,179,69,6,208,44,30,143,202,63,15,2,193,175,189,3,1,19,138,107,58,145,17,65,79,103,220,234,151,242,207,206,240,180,230,115,150,172,116,34,231,173,53,133,226,249,55,232,28,117,223,110,71,241,26,113,29,41,197,137,111,183,98,14,170,24,190,27,252,86,62,75,198,210,121,32,154,219,192,254,120,205,90,244,31,221,168,51,136,7,199,49,177,18,16,89,39,128,236,95,96,81,127,169,25,181,74,13,45,229,122,159,147,201,156,239,160,224,59,77,174,42,245,176,200,235,187,60,131,83,153,97,23,43,4,126,186,119,214,38,225,105,20,99,85,33,12,125],Nk=BS/32,Nb=BB/32,Nr=RA[Nk][Nb];