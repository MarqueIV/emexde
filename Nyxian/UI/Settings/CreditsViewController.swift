/*
 SPDX-License-Identifier: AGPL-3.0-or-later

 Copyright (C) 2025 - 2026 emexlab

 This file is part of Nyxian.

 Nyxian is free software: you can redistribute it and/or modify
 it under the terms of the GNU Affero General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 Nyxian is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 GNU Affero General Public License for more details.

 You should have received a copy of the GNU Affero General Public License
 along with Nyxian. If not, see <https://www.gnu.org/licenses/>.
*/

import Foundation
import UIKit

class CreditsViewController: UIThemedTableViewController {
    
    struct CreditSection {
        let title: String
        let credits: [Credit]
    }
    
    private let sections: [CreditSection] = [
        CreditSection(
            title: "Team",
            credits: [
                Credit(
                    name: "emexLabs",
                    role: "Maintainer",
                    githubURL: "https://github.com/emexlab"
                ),
                Credit(
                    name: "Nyxia",
                    role: "Maintainer",
                    githubURL: "https://github.com/mach-port-t"
                ),
                Credit(
                    name: "Catelyn",
                    role: "Developer",
                    githubURL: "https://github.com/mimalloc"
                ),
                Credit(
                    name: "LucaVmu",
                    role: "Developer",
                    githubURL: "https://github.com/lucavmu"
                ),
            ]
        ),
        
        CreditSection(
            title: "Security Researchers",
            credits: [
                Credit(
                    name: "semvis123",
                    role: "Security Researcher",
                    githubURL: "https://github.com/semvis123"
                ),
                Credit(
                    name: "zipgod",
                    role: "Security Researcher",
                    githubURL: "https://github.com/zipgod24"
                )
            ]
        ),
        
        CreditSection(
            title: "Contributors",
            credits: [
                Credit(
                    name: "Kyle",
                    role: "Swift Support",
                    githubURL: "https://github.com/Kyle-Ye"
                ),
                Credit(
                    name: "Vinogradov Daniil",
                    role: "LLVM-On-iOS",
                    githubURL: "https://github.com/XITRIX"
                ),
                Credit(
                    name: "엄세환",
                    role: "Contributor",
                    githubURL: "https://github.com/op06072"
                ),
                Credit(
                    name: "L0tsen",
                    role: "Contributor (What did she contribute again??)",
                    githubURL: "https://github.com/l0tsen"
                ),
                Credit(
                    name: "Offihito",
                    role: "Contributor (A vibecoder we banned)",
                    githubURL: "https://github.com/Offihito"
                )
            ]
        ),
        
        CreditSection(
            title: "Design & Icons",
            credits: [
                Credit(
                    name: "ayame09",
                    role: "Original Nyxian app icons",
                    githubURL: "https://github.com/ayayame09"
                ),
                Credit(
                    name: "sxdev",
                    role: "Drawn app icons",
                    githubURL: "https://github.com/SamoXcZ"
                ),
                Credit(
                    name: "xzadik",
                    role: "Nyxcat app icons",
                    githubURL: "https://github.com/xzadik"
                )
            ]
        ),
        
        CreditSection(
            title: "Open Source Projects",
            credits: [
                Credit(
                    name: "LiveContainer",
                    role: "LiveContainer",
                    githubURL: "https://github.com/livecontainer"
                ),
                Credit(
                    name: "LLVM",
                    role: "llvm-project",
                    githubURL: "https://github.com/llvm"
                ),
                Credit(
                    name: "The Swift Programming Language",
                    role: "swift",
                    githubURL: "https://github.com/swiftlang"
                )
            ]
        ),
        
        CreditSection(
            title: "Externals",
            credits: [
                Credit(
                    name: "Duy Tran",
                    role: "The guy who said that NSXPC services don't need to be thread safe.",
                    githubURL: "https://github.com/khanhduytran0"
                ),
                Credit(
                    name: "Simon Støvring",
                    role: "Runestone",
                    githubURL: "https://github.com/simonbs"
                ),
                Credit(
                    name: "light-tech",
                    role: "LLVM-On-iOS",
                    githubURL: "https://github.com/light-tech"
                ),
                Credit(
                    name: "Lars Fröder",
                    role: "Litehook",
                    githubURL: "https://github.com/opa334"
                ),
                Credit(
                    name: "lascic",
                    role: "UIOnboarding",
                    githubURL: "https://github.com/lascic"
                ),
                Credit(
                    name: "Miguel de Icaza",
                    role: "SwiftTerm",
                    githubURL: "https://github.com/migueldeicaza"
                )
            ]
        )
    ]
    
    override func viewDidLoad() {
        super.viewDidLoad()
        
        self.title = "Credits"
        
        self.tableView.register(CreditCell.self, forCellReuseIdentifier: CreditCell.identifier)
    }
    
    override func numberOfSections(in tableView: UITableView) -> Int {
        sections.count
    }
    
    override func tableView(_ tableView: UITableView, numberOfRowsInSection section: Int) -> Int {
        sections[section].credits.count
    }
    
    override func tableView(_ tableView: UITableView, titleForHeaderInSection section: Int) -> String? {
        sections[section].title
    }
    
    override func tableView(_ tableView: UITableView, heightForRowAt indexPath: IndexPath) -> CGFloat {
        if #available(iOS 26.0, *) {
            return 90
        } else {
            return 80
        }
    }
    
    override func tableView(_ tableView: UITableView, cellForRowAt indexPath: IndexPath) -> UITableViewCell {
        guard let cell = tableView.dequeueReusableCell(withIdentifier: CreditCell.identifier, for: indexPath) as? CreditCell else {
            return UITableViewCell()
        }
        
        let credit = sections[indexPath.section].credits[indexPath.row]
        
        cell.nameLabel.text = credit.name
        cell.roleLabel.text = credit.role
        
        downloadImage(from: "\(credit.githubURL).png") { image in
            cell.configureImage(image ?? UIImage(systemName: "person.fill"))
            cell.layoutSubviews()
        }
        
        return cell
    }
    
    func downloadImage(from urlString: String, completion: @escaping (UIImage?) -> Void) {
        guard let url = URL(string: urlString) else {
            completion(nil)
            return
        }
        
        URLSession.shared.dataTask(with: url) { data, _, error in
            if let data = data, let image = UIImage(data: data) {
                DispatchQueue.main.async { completion(image) }
            } else {
                DispatchQueue.main.async { completion(nil) }
            }
        }.resume()
    }
    
    override func tableView(_ tableView: UITableView, didSelectRowAt indexPath: IndexPath) {
        let credit = sections[indexPath.section].credits[indexPath.row]
        
        if let url = URL(string: credit.githubURL) {
            UIApplication.shared.open(url)
        }
        
        tableView.deselectRow(at: indexPath, animated: true)
    }
}
