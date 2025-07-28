import './pretend-to-be-a-browser'
import  {CreatePublicTestSpace, CreateTestUser, LoginAsUser, LaunchTestPage, DeleteSpace, LogoutUser, TEST_ACCOUNT_PASSWORD} from './testhelpers'

import { test } from 'uvu';
import * as assert from 'uvu/assert';
import { CSPFoundation, ready, Systems } from 'connected-spaces-platform.web';
import { initializeCSP } from './shared/csp-initializer.js';
import * as fs from 'fs';

//Initialize CSPFoundation before the tests run
//True if USE_RELEASE_CSP is not set, false otherwise. Idea here is we want debug to be the default mode.
//const USE_DEBUG_CSP: boolean = process.env.USE_RELEASE_CSP === undefined;
const USE_DEBUG_CSP = false;

test.before(async () => {
  return initializeCSP(USE_DEBUG_CSP); //gotta return the promise or tests wont automatically await
});


test ('EnterSpaceBenchmark', async() => {
  const user = await CreateTestUser();
  await LoginAsUser(user);
  const spaceId = await CreatePublicTestSpace();
  const {errors, consoleMessages} = await LaunchTestPage('http://127.0.0.1:8888/EnterSpaceBenchmark.html', USE_DEBUG_CSP, null, spaceId)


  console.log(consoleMessages);
  
  const interestingTags = ["EnterSpace:System", "GetGroupId (GetSpace)", "GetGroupId (GetSpace):ClientSideCallbackProcessing", "GroupCodesUserPut (Add User To Space)", "RefreshMultiplayerScopes", "RetrieveAllEntities", "RetreiveAllEntities"];
  const profileTimestamps = consoleMessages.filter(s => s.startsWith("PROFILESTART") ||
                                                        s.startsWith("PROFILEEND") &&
                                                        interestingTags.some(tag => s.includes(tag)));

  console.log(profileTimestamps);

  console.log("Writing timestamps to EnterSpaceBenchmarkTimestamps.txt");
  fs.writeFileSync("EnterSpaceBenchmarkTimestamps.txt", profileTimestamps.join(`\n`));

  //Cleanup
  await DeleteSpace(spaceId);
  await LogoutUser(user);
})


test.run();

