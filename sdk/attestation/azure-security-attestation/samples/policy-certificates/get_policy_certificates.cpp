// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

/**
 * @brief This sample demonstrates using the Attestation Service SDK for C++ to retrieve policy
 * management certificates from an Isolated mode attestation service instance.
 *
 * Remember that when an attestation service instance is in isolated mode, the service is configured
 * with a set of X.509 certificates. The GetIsolatedModeCertificates API returns a list of the
 * existing certificates.
 *
 * @remark The following environment variables must be set before running the sample.
 * - ATTESTATION_ISOLATED_URL:  Points to an Attestation Service Instance in Isolated mode.
 * operations.
 * - AZURE_TENANT_ID:     Tenant ID for the Azure account.
 * - AZURE_CLIENT_ID:     The Client ID to authenticate the request.
 * - AZURE_CLIENT_SECRET: The client secret.
 *
 */

#include "cryptohelpers.hpp"

#include <azure/attestation.hpp>
#include <azure/identity.hpp>

#include <chrono>
#include <get_env.hpp>
#include <iomanip>
#include <iostream>
#include <thread>
#include <vector>

// cspell:: words mrsigner mrenclave mitm
using namespace Azure::Security::Attestation;
using namespace Azure::Security::Attestation::Models;
using namespace std::chrono_literals;
using namespace Azure::Core;

int main()
{
  try
  {
    // create an administration client
    auto const credential = std::make_shared<Azure::Identity::ClientSecretCredential>(
        GetEnvHelper::GetEnv("AZURE_TENANT_ID"),
        GetEnvHelper::GetEnv("AZURE_CLIENT_ID"),
        GetEnvHelper::GetEnv("AZURE_CLIENT_SECRET"));
    AttestationAdministrationClient adminClient(AttestationAdministrationClient::Create(
        GetEnvHelper::GetEnv("ATTESTATION_ISOLATED_URL"), credential));

    // Retrieve the SGX Attestation Policy from this attestation service instance.
    Azure::Response<AttestationToken<IsolatedModeCertificateListResult>> const policyCertificates
        = adminClient.GetIsolatedModeCertificates();

    std::cout << "There are " << policyCertificates.Value.Body.Certificates.size()
              << " certificates configured on this instance." << std::endl;

    std::cout << "Enumerating policy certificates:" << std::endl;
    for (const auto& certChain : policyCertificates.Value.Body.Certificates)
    {
      // Retrieve the leaf certificate from the chain and parse it as an X.509
      // certificate.
      // RFC 7515 specifies that the leaf certificate of a certificate chain MUST be the first
      // certificate in the certificate chain, other certificates MAY follow the leaf certificate.
      std::string firstCertificateInChain((*certChain.CertificateChain)[0]);
      std::unique_ptr<::Cryptography::X509Certificate> const x509Cert(
          ::Cryptography::ImportX509Certificate(firstCertificateInChain));

      // Dump the subject and issuer of that certificate.
      std::cout << "Subject of signing certificate is: " << x509Cert->GetSubjectName() << std::endl;
      std::cout << "Issuer of signing certificate is: " << x509Cert->GetIssuerName() << std::endl;
    }
  }
  catch (Azure::Core::Credentials::AuthenticationException const& e)
  {
    std::cout << "Authentication Exception happened:" << std::endl << e.what() << std::endl;
    return 1;
  }
  catch (Azure::Core::RequestFailedException const& e)
  {
    std::cout << "Request Failed Exception happened:" << std::endl << e.what() << std::endl;
    if (e.RawResponse)
    {
      std::cout << "Error Code: " << e.ErrorCode << std::endl;
      std::cout << "Error Message: " << e.Message << std::endl;
    }
    return 1;
  }
  return 0;
}
