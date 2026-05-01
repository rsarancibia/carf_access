using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace LicenseGenerator
{
    internal class LicenseData
    {
        private const string cCOMPANY_NAME  = "EDV";


        public long _expirationDate(int days)
        {
            /*
            int dias = 7;

DateTime expiryUtc = DateTime.UtcNow.AddDays(dias);

long expiryEpoch = ((DateTimeOffset)expiryUtc).ToUnixTimeSeconds();

Console.WriteLine(expiryEpoch);
            */

            return DateTimeOffset.UtcNow.AddDays(days).ToUnixTimeSeconds();
        }

    }
}
